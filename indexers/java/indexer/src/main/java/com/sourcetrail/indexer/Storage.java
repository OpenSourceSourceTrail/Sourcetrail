package com.sourcetrail.indexer;

import com.github.javaparser.ast.Node;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import sourcetrail.SourcetrailCommon;
import sourcetrail.SourcetrailCommon.StorageComponentAccess;
import sourcetrail.SourcetrailCommon.StorageEdge;
import sourcetrail.SourcetrailCommon.StorageFile;
import sourcetrail.SourcetrailCommon.StorageLocalSymbol;
import sourcetrail.SourcetrailCommon.StorageNode;
import sourcetrail.SourcetrailCommon.StorageOccurrence;
import sourcetrail.SourcetrailCommon.StorageSourceLocation;
import sourcetrail.SourcetrailCommon.StorageSymbol;

/**
 * Buffers proto messages and emits a single {@link SourcetrailCommon.IntermediateStorage}
 * for the engine to merge.
 *
 * <p>Ids start at 1 and increase monotonically, matching the C++ engine convention
 * ({@code mNextId = 1}). The {@code nextId} field on the emitted message is the next
 * id to be allocated.
 *
 * <p>Node identity for merge-time dedup is ({@code type, serializedName}); the
 * C++ {@code Storage::inject} step remaps ids across worker processes
 * (see {@code src/lib/lib/data/storage/Storage.cpp}). This class dedups on the same key
 * <em>within</em> a file, so a symbol referenced fifty times produces one node, and a reference to
 * something declared in this same file lands on the declaration's id.
 */
public final class Storage {
  /** Id assigned to the file node of the current index. */
  public static final long FILE_ID = 1;

  private final List<StorageNode> nodes = new ArrayList<>();
  private final List<StorageSymbol> symbols = new ArrayList<>();
  private final List<StorageEdge> edges = new ArrayList<>();
  private final List<StorageLocalSymbol> localSymbols = new ArrayList<>();
  private final List<StorageSourceLocation> sourceLocations = new ArrayList<>();
  private final List<StorageOccurrence> occurrences = new ArrayList<>();
  private final List<StorageComponentAccess> componentAccesses = new ArrayList<>();
  private final List<SourcetrailCommon.StorageError> errors = new ArrayList<>();

  private final StorageFile.Builder file;
  private long nextId = FILE_ID + 1;

  /** JavaParser AST node to assigned id, so references/edges can point at the definition. */
  private final IdentityHashMap<Node, Long> idByAst = new IdentityHashMap<>();

  /** (type, serializedName) to id, the same key the engine dedups on at merge time. */
  private final Map<String, Long> idByName = new HashMap<>();

  /** Local symbol name to id, so every use of a variable shares one StorageLocalSymbol. */
  private final Map<String, Long> localSymbolIds = new HashMap<>();

  public Storage(String filePath) {
    this.file = StorageFile.newBuilder()
        .setId(FILE_ID)
        .setFilePath(filePath == null ? "" : filePath)
        .setLanguageIdentifier("Java")
        .setIndexed(true)
        .setComplete(true);
    // Emit the file node (the read path references it from StorageFile and
    // location.fileNodeId). It has NODE_FILE type and a /-delimited name.
    nodes.add(StorageNode.newBuilder()
        .setId(FILE_ID)
        .setType(Kinds.NODE_FILE)
        .setSerializedName(Names.file(filePath))
        .build());
  }

  public long fileId() {
    return FILE_ID;
  }

  /** Clear on a parse error or an aborted walk, so the engine keeps the file on the retry list. */
  public void setFileComplete(boolean complete) {
    file.setComplete(complete);
  }

  private long allocId() {
    return nextId++;
  }

  private static String nameKey(int nodeType, String serializedName) {
    return nodeType + " " + serializedName;
  }

  // ---- record methods ---------------------------------------------

  /** Assign an id (idempotent per AST node and per name) and emit one node for a declaration. */
  public long nodeFor(Node ast, int nodeType, Names.Element... nameChain) {
    return nodeFor(ast, nodeType, Names.join(nameChain));
  }

  /** As above, for a caller that already holds the serialized name. */
  public long nodeFor(Node ast, int nodeType, String serializedName) {
    Long already = idByAst.get(ast);
    if(already != null) {
      return already;
    }
    long id = nodeByName(nodeType, serializedName);
    idByAst.put(ast, id);
    return id;
  }

  /**
   * Id for a node identified only by its serialized name - a reference to a symbol whose
   * declaration may live in another file. Repeated lookups return the same id, and a name already
   * claimed by a declaration in this file resolves to that declaration.
   */
  public long nodeByName(int nodeType, String serializedName) {
    Long already = idByName.get(nameKey(nodeType, serializedName));
    if(already != null) {
      return already;
    }
    long id = allocId();
    idByName.put(nameKey(nodeType, serializedName), id);
    nodes.add(StorageNode.newBuilder().setId(id).setType(nodeType).setSerializedName(serializedName).build());
    return id;
  }

  public long nodeByName(int nodeType, Names.Element... nameChain) {
    return nodeByName(nodeType, Names.join(nameChain));
  }

  public boolean known(Node ast) {
    return idByAst.containsKey(ast);
  }

  public long idOf(Node ast) {
    Long id = idByAst.get(ast);
    return id == null ? 0 : id;
  }

  public void symbol(long id, int definitionKind) {
    if(definitionKind != Kinds.DEFINITION_NONE) {
      symbols.add(StorageSymbol.newBuilder().setId(id).setDefinitionKind(definitionKind).build());
    }
  }

  public void access(long id, int accessKind) {
    if(accessKind != Kinds.ACCESS_NONE) {
      componentAccesses.add(StorageComponentAccess.newBuilder().setNodeId(id).setType(accessKind).build());
    }
  }

  /**
   * Emit an edge and return its id. The id matters: a reference is only navigable in the GUI once a
   * {@link StorageOccurrence} ties a source location to the <em>edge</em>, so every caller that
   * knows the reference's range should follow this with {@link #location}.
   *
   * @return the new edge's id, or 0 if the edge was not emitted
   */
  public long edge(long src, long dst, int edgeType) {
    if(src == 0 || dst == 0 || src == dst) {
      return 0;
    }
    long id = allocId();
    edges.add(StorageEdge.newBuilder()
        .setId(id)
        .setType(edgeType)
        .setSourceNodeId(src)
        .setTargetNodeId(dst)
        .build());
    return id;
  }

  /**
   * Id for a function-local variable or type parameter. Names follow the C++ convention from
   * {@code CxxAstVisitorComponentIndexer::getLocalSymbolName} - the file name plus the declaration's
   * line:col - so every use of one variable shares a single symbol.
   */
  public long localSymbol(String name) {
    Long already = localSymbolIds.get(name);
    if(already != null) {
      return already;
    }
    long id = allocId();
    localSymbolIds.put(name, id);
    localSymbols.add(StorageLocalSymbol.newBuilder().setId(id).setName(name).build());
    return id;
  }

  public void location(long elementId, long line, long col, long endLine, long endCol, int type) {
    if(elementId == 0) {
      return;
    }
    long locId = allocId();
    sourceLocations.add(StorageSourceLocation.newBuilder()
        .setId(locId)
        .setFileNodeId(FILE_ID)
        .setStartLine(line)
        .setStartCol(col)
        .setEndLine(endLine)
        .setEndCol(endCol)
        .setType(type)
        .build());
    occurrences.add(StorageOccurrence.newBuilder().setElementId(elementId).setSourceLocationId(locId).build());
  }

  public void error(String message, String translationUnit) {
    long id = allocId();
    errors.add(SourcetrailCommon.StorageError.newBuilder()
        .setId(id)
        .setMessage(message == null ? "" : message)
        .setTranslationUnit(translationUnit == null ? "" : translationUnit)
        .setFatal(false)
        .setIndexed(true)
        .build());
  }

  // ---- Build -------------------------------------------------------

  public SourcetrailCommon.IntermediateStorage build() {
    SourcetrailCommon.IntermediateStorage.Builder b = SourcetrailCommon.IntermediateStorage.newBuilder()
        .setNextId(nextId)
        .addFiles(file.build());
    nodes.forEach(b::addNodes);
    symbols.forEach(b::addSymbols);
    edges.forEach(b::addEdges);
    localSymbols.forEach(b::addLocalSymbols);
    sourceLocations.forEach(b::addSourceLocations);
    occurrences.forEach(b::addOccurrences);
    componentAccesses.forEach(b::addComponentAccesses);
    errors.forEach(b::addErrors);
    return b.build();
  }
}
