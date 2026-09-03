package com.sourcetrail.indexer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import sourcetrail.SourcetrailCommon.IntermediateStorage;
import sourcetrail.SourcetrailCommon.StorageComponentAccess;
import sourcetrail.SourcetrailCommon.StorageEdge;
import sourcetrail.SourcetrailCommon.StorageError;
import sourcetrail.SourcetrailCommon.StorageFile;
import sourcetrail.SourcetrailCommon.StorageLocalSymbol;
import sourcetrail.SourcetrailCommon.StorageNode;
import sourcetrail.SourcetrailCommon.StorageOccurrence;
import sourcetrail.SourcetrailCommon.StorageSourceLocation;

/**
 * Java port of the C++ test helper {@code TestStorage}
 * ({@code src/lib/lib/tests/helper/testStorage/TestStorage.cpp}/{@code .h}).
 *
 * <p>The engine's own integration tests flatten a {@code Storage} into short, human-readable
 * strings such as {@code "public void Foo.m() <3:8 3:9>"} instead of asserting on raw ids. This
 * class produces the same strings from an {@code IntermediateStorage} proto so Java indexer tests
 * can read the same way (see {@code tests/integration/lib_cxx/CxxParser11TestSuite.cpp} for the
 * C++-side style this mirrors). Every formatting rule here (name rendering, access prefixing,
 * location nesting, node/edge bin routing) ports the C++ source line for line; read that file
 * before changing this one.
 */
public final class TestStorage {
  private final IntermediateStorage proto;

  public final List<String> packages = new ArrayList<>();
  public final List<String> classes = new ArrayList<>();
  public final List<String> interfaces = new ArrayList<>();
  public final List<String> annotations = new ArrayList<>();
  public final List<String> enums = new ArrayList<>();
  public final List<String> enumConstants = new ArrayList<>();
  public final List<String> methods = new ArrayList<>();
  public final List<String> fields = new ArrayList<>();
  public final List<String> functions = new ArrayList<>();
  public final List<String> typeParameters = new ArrayList<>();
  public final List<String> namespaces = new ArrayList<>();
  public final List<String> localSymbols = new ArrayList<>();
  public final List<String> qualifiers = new ArrayList<>();

  public final List<String> calls = new ArrayList<>();
  public final List<String> inheritances = new ArrayList<>();
  public final List<String> overrides = new ArrayList<>();
  public final List<String> usages = new ArrayList<>();
  public final List<String> typeUses = new ArrayList<>();
  public final List<String> typeArguments = new ArrayList<>();
  public final List<String> imports = new ArrayList<>();
  public final List<String> annotationUses = new ArrayList<>();

  public final List<String> errors = new ArrayList<>();
  public final Set<String> files = new HashSet<>();

  private TestStorage(IntermediateStorage proto) {
    this.proto = proto;
  }

  public static TestStorage create(IntermediateStorage proto) {
    TestStorage storage = new TestStorage(proto);
    storage.build();
    return storage;
  }

  // ---- build ----------------------------------------------------------

  private void build() {
    for(StorageFile file : proto.getFilesList()) {
      files.add(file.getFilePath());
    }

    Map<Long, Integer> accessByNodeId = new HashMap<>();
    for(StorageComponentAccess access : proto.getComponentAccessesList()) {
      accessByNodeId.put(access.getNodeId(), access.getType());
    }

    Map<Long, List<Long>> elementIdsByLocationId = new HashMap<>();
    for(StorageOccurrence occurrence : proto.getOccurrencesList()) {
      elementIdsByLocationId.computeIfAbsent(occurrence.getSourceLocationId(), k -> new ArrayList<>())
          .add(occurrence.getElementId());
    }

    Map<Long, List<StorageSourceLocation>> tokenLocations = new HashMap<>();
    Map<Long, List<StorageSourceLocation>> scopeLocations = new HashMap<>();
    Map<Long, List<StorageSourceLocation>> signatureLocations = new HashMap<>();
    Map<Long, List<StorageSourceLocation>> localSymbolLocations = new HashMap<>();
    Map<Long, List<StorageSourceLocation>> qualifierLocations = new HashMap<>();
    Map<Long, List<StorageSourceLocation>> errorLocations = new HashMap<>();

    for(StorageSourceLocation location : proto.getSourceLocationsList()) {
      List<Long> elementIds = elementIdsByLocationId.get(location.getId());
      if(elementIds == null || elementIds.isEmpty()) {
        continue;
      }
      Map<Long, List<StorageSourceLocation>> target = switch((int) location.getType()) {
        case Kinds.LOCATION_TOKEN -> tokenLocations;
        case Kinds.LOCATION_SCOPE -> scopeLocations;
        case Kinds.LOCATION_QUALIFIER -> qualifierLocations;
        case Kinds.LOCATION_LOCAL_SYMBOL -> localSymbolLocations;
        case Kinds.LOCATION_SIGNATURE -> signatureLocations;
        case Kinds.LOCATION_ERROR -> errorLocations;
        default -> null;
      };
      if(target == null) {
        continue;
      }
      for(long elementId : elementIds) {
        if(elementId != 0) {
          target.computeIfAbsent(elementId, k -> new ArrayList<>()).add(location);
        }
      }
    }

    Map<Long, StorageNode> nodesById = new HashMap<>();
    Set<Long> fileNodeIds = new HashSet<>();
    for(StorageNode node : proto.getNodesList()) {
      nodesById.put(node.getId(), node);
      if(node.getType() == Kinds.NODE_FILE) {
        fileNodeIds.add(node.getId());
      }

      String nameStr = renderName(node.getSerializedName());

      for(StorageSourceLocation loc : qualifierLocations.getOrDefault(node.getId(), List.of())) {
        qualifiers.add(nameStr + addLocationStr("", loc));
      }

      List<String> bin = binForNodeType(node.getType());
      if(bin == null) {
        continue;
      }

      Integer access = accessByNodeId.get(node.getId());
      if(access != null && access != Kinds.ACCESS_TEMPLATE_PARAMETER && access != Kinds.ACCESS_TYPE_PARAMETER) {
        nameStr = accessKindToString(access) + " " + nameStr;
      }

      List<StorageSourceLocation> tokenLocs = tokenLocations.get(node.getId());
      boolean added = false;
      if(tokenLocs != null) {
        for(StorageSourceLocation tokenLoc : tokenLocs) {
          added = false;
          String locationStr = addLocationStr("", tokenLoc);

          StorageSourceLocation signatureLoc = firstOrNull(signatureLocations.get(node.getId()));
          if(signatureLoc != null && containsLocation(signatureLoc, tokenLoc)) {
            locationStr = addLocationStr(locationStr, signatureLoc);
          }

          for(StorageSourceLocation scopeLoc : scopeLocations.getOrDefault(node.getId(), List.of())) {
            if(containsLocation(scopeLoc, tokenLoc)) {
              bin.add(nameStr + addLocationStr(locationStr, scopeLoc));
              added = true;
            }
          }

          if(!added) {
            bin.add(nameStr + locationStr);
            added = true;
          }
        }
      }

      if(!added) {
        bin.add(nameStr);
      }
    }

    for(StorageEdge edge : proto.getEdgesList()) {
      StorageNode target = nodesById.get(edge.getTargetNodeId());
      StorageNode source = nodesById.get(edge.getSourceNodeId());
      if(target == null || source == null) {
        continue;
      }

      List<String> bin = binForEdgeType(edge.getType());
      if(bin == null) {
        continue;
      }

      String sourceName = renderName(source.getSerializedName());
      if(fileNodeIds.contains(edge.getSourceNodeId())) {
        sourceName = fileName(sourceName);
      }
      String targetName = renderName(target.getSerializedName());
      if(fileNodeIds.contains(edge.getTargetNodeId())) {
        targetName = fileName(targetName);
      }
      String nameStr = sourceName + " -> " + targetName;

      List<StorageSourceLocation> tokenLocs = tokenLocations.get(edge.getId());
      boolean added = false;
      if(tokenLocs != null) {
        for(StorageSourceLocation tokenLoc : tokenLocs) {
          bin.add(nameStr + addLocationStr("", tokenLoc));
          added = true;
        }
      }

      if(!added) {
        bin.add(nameStr);
      }
    }

    for(StorageLocalSymbol localSymbol : proto.getLocalSymbolsList()) {
      List<StorageSourceLocation> locs = localSymbolLocations.get(localSymbol.getId());
      boolean added = false;
      if(locs != null) {
        for(StorageSourceLocation loc : locs) {
          localSymbols.add(localSymbol.getName() + addLocationStr("", loc));
          added = true;
        }
      }
      if(!added) {
        localSymbols.add(localSymbol.getName());
      }
    }

    for(StorageError error : proto.getErrorsList()) {
      for(StorageSourceLocation loc : errorLocations.getOrDefault(error.getId(), List.of())) {
        errors.add(error.getMessage() + addLocationStr("", loc));
      }
    }
  }

  // ---- formatting helpers ---------------------------------------------

  /** Renders a {@code Names}-serialized name the way {@code NameHierarchy::getQualifiedNameWithSignature} does. */
  private static String renderName(String serialized) {
    int mIdx = serialized.indexOf("\tm");
    String delimiter = serialized.substring(0, mIdx);
    String[] chunks = serialized.substring(mIdx + 2).split("\tn", -1);
    List<String> names = new ArrayList<>(chunks.length);
    String prefix = "";
    String postfix = "";
    for(int i = 0; i < chunks.length; i++) {
      String chunk = chunks[i];
      int sIdx = chunk.indexOf("\ts");
      int pIdx = chunk.indexOf("\tp", sIdx);
      names.add(chunk.substring(0, sIdx));
      if(i == chunks.length - 1) {
        prefix = chunk.substring(sIdx + 2, pIdx);
        postfix = chunk.substring(pIdx + 2);
      }
    }
    String qualified = String.join(delimiter, names);
    return prefix + (prefix.isEmpty() ? "" : " ") + qualified + postfix;
  }

  private static String addLocationStr(String inner, StorageSourceLocation loc) {
    return " <" + loc.getStartLine() + ":" + loc.getStartCol() + inner + " " + loc.getEndLine() + ":" +
        loc.getEndCol() + ">";
  }

  private static boolean containsLocation(StorageSourceLocation outLocation, StorageSourceLocation inLocation) {
    if(outLocation.getStartLine() > inLocation.getStartLine()) {
      return false;
    }
    if(outLocation.getStartLine() == inLocation.getStartLine() && outLocation.getStartCol() > inLocation.getStartCol()) {
      return false;
    }
    if(outLocation.getEndLine() < inLocation.getEndLine()) {
      return false;
    }
    if(outLocation.getEndLine() == inLocation.getEndLine() && outLocation.getEndCol() < inLocation.getEndCol()) {
      return false;
    }
    return true;
  }

  /** Port of {@code FilePath::fileName()} for the one thing we need it for: the last path segment. */
  private static String fileName(String path) {
    int idx = Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
    return idx < 0 ? path : path.substring(idx + 1);
  }

  /** Port of {@code accessKindToString} (AccessKind.cpp). */
  private static String accessKindToString(int access) {
    return switch(access) {
      case Kinds.ACCESS_PUBLIC -> "public";
      case Kinds.ACCESS_PROTECTED -> "protected";
      case Kinds.ACCESS_PRIVATE -> "private";
      case Kinds.ACCESS_DEFAULT -> "default";
      default -> "";
    };
  }

  private List<String> binForNodeType(int type) {
    if(type == Kinds.NODE_PACKAGE) {
      return packages;
    }
    if(type == Kinds.NODE_CLASS) {
      return classes;
    }
    if(type == Kinds.NODE_INTERFACE) {
      return interfaces;
    }
    if(type == Kinds.NODE_ANNOTATION) {
      return annotations;
    }
    if(type == Kinds.NODE_ENUM) {
      return enums;
    }
    if(type == Kinds.NODE_ENUM_CONSTANT) {
      return enumConstants;
    }
    if(type == Kinds.NODE_METHOD) {
      return methods;
    }
    if(type == Kinds.NODE_FIELD) {
      return fields;
    }
    if(type == Kinds.NODE_FUNCTION) {
      return functions;
    }
    if(type == Kinds.NODE_TYPE_PARAMETER) {
      return typeParameters;
    }
    if(type == Kinds.NODE_NAMESPACE) {
      return namespaces;
    }
    return null;
  }

  private List<String> binForEdgeType(int type) {
    if(type == Kinds.EDGE_CALL) {
      return calls;
    }
    if(type == Kinds.EDGE_INHERITANCE) {
      return inheritances;
    }
    if(type == Kinds.EDGE_OVERRIDE) {
      return overrides;
    }
    if(type == Kinds.EDGE_USAGE) {
      return usages;
    }
    if(type == Kinds.EDGE_TYPE_USAGE) {
      return typeUses;
    }
    if(type == Kinds.EDGE_TYPE_ARGUMENT) {
      return typeArguments;
    }
    if(type == Kinds.EDGE_IMPORT) {
      return imports;
    }
    if(type == Kinds.EDGE_ANNOTATION_USAGE) {
      return annotationUses;
    }
    return null;
  }

  private static <T> T firstOrNull(List<T> list) {
    return (list == null || list.isEmpty()) ? null : list.get(0);
  }

  // ---- proto-level helpers (lifted from JavaIndexerTest) ---------------

  public StorageNode node(int type, String nameContains) {
    return proto.getNodesList().stream()
        .filter(n -> n.getType() == type && n.getSerializedName().contains(nameContains))
        .findFirst()
        .orElseThrow(() -> new AssertionError(
            "no node type=" + type + " containing \"" + nameContains + "\" in " + names()));
  }

  public boolean hasNode(int type) {
    return proto.getNodesList().stream().anyMatch(n -> n.getType() == type);
  }

  public boolean hasEdge(long from, long to, int type) {
    return proto.getEdgesList().stream()
        .anyMatch(e -> e.getSourceNodeId() == from && e.getTargetNodeId() == to && e.getType() == type);
  }

  public boolean hasEdgeFrom(long from, int type) {
    return proto.getEdgesList().stream().anyMatch(e -> e.getSourceNodeId() == from && e.getType() == type);
  }

  public List<StorageEdge> edgesOf(int type) {
    return proto.getEdgesList().stream().filter(e -> e.getType() == type).toList();
  }

  /** Location types attached to an element id, via the occurrence table. */
  public Set<Integer> locationTypesOf(long elementId) {
    Set<Long> locIds = proto.getOccurrencesList().stream()
        .filter(o -> o.getElementId() == elementId)
        .map(StorageOccurrence::getSourceLocationId)
        .collect(HashSet::new, Set::add, Set::addAll);
    return proto.getSourceLocationsList().stream()
        .filter(l -> locIds.contains(l.getId()))
        .map(l -> (int) l.getType())
        .collect(HashSet::new, Set::add, Set::addAll);
  }

  public boolean hasSymbol(long id) {
    return proto.getSymbolsList().stream().anyMatch(sym -> sym.getId() == id);
  }

  public int accessOf(long id) {
    return proto.getComponentAccessesList().stream()
        .filter(a -> a.getNodeId() == id)
        .map(a -> (int) a.getType())
        .findFirst()
        .orElse(Kinds.ACCESS_NONE);
  }

  /** Raw serialized names, for diagnostics -- {@code AssertionError} messages, "must appear once" checks. */
  public List<String> names() {
    return proto.getNodesList().stream().map(StorageNode::getSerializedName).toList();
  }

  public IntermediateStorage proto() {
    return proto;
  }
}
