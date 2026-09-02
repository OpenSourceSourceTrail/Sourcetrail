package com.sourcetrail.indexer;

import com.github.javaparser.ast.Node;
import java.util.ArrayList;
import java.util.IdentityHashMap;
import java.util.List;
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
 * (see {@code src/lib/lib/data/storage/Storage.cpp}).
 */
public final class Storage {
  /** Id assigned to the file node of the current index. */
  public static final long FILE_ID = 1;

  private final List<StorageNode> nodes = new ArrayList<>();
  private final List<StorageFile> files = new ArrayList<>();
  private final List<StorageSymbol> symbols = new ArrayList<>();
  private final List<StorageEdge> edges = new ArrayList<>();
  private final List<StorageLocalSymbol> localSymbols = new ArrayList<>();
  private final List<StorageSourceLocation> sourceLocations = new ArrayList<>();
  private final List<StorageOccurrence> occurrences = new ArrayList<>();
  private final List<StorageComponentAccess> componentAccesses = new ArrayList<>();
  private final List<SourcetrailCommon.StorageError> errors = new ArrayList<>();

  private long nextId = FILE_ID + 1;

  /** JavaParser AST node → assigned id, so references/edges can point at the definition. */
  private final IdentityHashMap<Node, Long> idByAst = new IdentityHashMap<>();

  public Storage(String filePath) {
    files.add(StorageFile.newBuilder()
        .setId(FILE_ID)
        .setFilePath(filePath == null ? "" : filePath)
        .setLanguageIdentifier("Java")
        .setIndexed(true)
        .setComplete(true)
        .build());
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

  private long allocId() {
    return nextId++;
  }

  // ---- record methods ---------------------------------------------

  /** Assign an id (idempotent) and emit one node for an AST node. */
  public long nodeFor(Node ast, int nodeType, Names.Element... nameChain) {
    Long already = idByAst.get(ast);
    if(already != null) {
      return already;
    }
    long id = allocId();
    idByAst.put(ast, id);
    nodes.add(StorageNode.newBuilder()
        .setId(id)
        .setType(nodeType)
        .setSerializedName(Names.join(nameChain))
        .build());
    return id;
  }

  /** Fresh id + node emission for an "unanchored" node. */
  public long nodeUnanchored(int nodeType, Names.Element... nameChain) {
    long id = allocId();
    nodes.add(StorageNode.newBuilder()
        .setId(id)
        .setType(nodeType)
        .setSerializedName(Names.join(nameChain))
        .build());
    return id;
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

  public void edge(long src, long dst, int edgeType) {
    if(src == 0 || dst == 0) {
      return;
    }
    edges.add(StorageEdge.newBuilder()
        .setId(allocId())
        .setType(edgeType)
        .setSourceNodeId(src)
        .setTargetNodeId(dst)
        .build());
  }

  public long localSymbol(String name) {
    long id = allocId();
    localSymbols.add(StorageLocalSymbol.newBuilder().setId(id).setName(name).build());
    return id;
  }

  public void location(long elementId, long line, long col, long endLine, long endCol, int type) {
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
        .setNextId(nextId);
    files.forEach(b::addFiles);
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
