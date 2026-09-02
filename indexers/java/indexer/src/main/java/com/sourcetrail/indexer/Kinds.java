package com.sourcetrail.indexer;

/**
 * Integer constants mirroring the C++ enums the engine uses. Values must stay in sync
 * with the equivalent int values in:
 *   src/lib/lib/data/NodeKind.h            (NodeKind / StorageNode.type)
 *   src/lib/lib/data/DefinitionKind.h      (DefinitionKind / StorageSymbol.definition_kind)
 *   src/lib/lib/data/parser/AccessKind.h   (AccessKind / StorageComponentAccess.type)
 *   src/lib/lib/data/graph/Edge.h          (Edge::EdgeType / StorageEdge.type)
 *   src/lib/lib/data/location/LocationType.h (LocationType / StorageSourceLocation.type)
 *   src/lib/lib/data/parser/ReferenceKind.h (ReferenceKind — used internally only)
 */
public final class Kinds {
  private Kinds() {}

  public static final int NODE_SYMBOL = 1 << 0;
  public static final int NODE_CLASS = 1 << 7;
  public static final int NODE_INTERFACE = 1 << 8;
  public static final int NODE_ANNOTATION = 1 << 9;
  public static final int NODE_FIELD = 1 << 11;
  public static final int NODE_FUNCTION = 1 << 12;
  public static final int NODE_METHOD = 1 << 13;
  public static final int NODE_ENUM = 1 << 14;
  public static final int NODE_ENUM_CONSTANT = 1 << 15;
  public static final int NODE_PACKAGE = 1 << 5;
  public static final int NODE_TYPE_PARAMETER = 1 << 17;
  public static final int NODE_FILE = 1 << 18;
  public static final int NODE_NAMESPACE = 1 << 4;

  public static final int DEFINITION_NONE = 0;
  public static final int DEFINITION_IMPLICIT = 1;
  public static final int DEFINITION_EXPLICIT = 2;

  public static final int ACCESS_NONE = 0;
  public static final int ACCESS_PUBLIC = 1;
  public static final int ACCESS_PROTECTED = 2;
  public static final int ACCESS_PRIVATE = 3;
  public static final int ACCESS_DEFAULT = 4;
  public static final int ACCESS_TEMPLATE_PARAMETER = 5;
  public static final int ACCESS_TYPE_PARAMETER = 6;

  public static final int EDGE_UNDEFINED = 0;
  public static final int EDGE_MEMBER = 1 << 0;
  public static final int EDGE_TYPE_USAGE = 1 << 1;
  public static final int EDGE_USAGE = 1 << 2;
  public static final int EDGE_CALL = 1 << 3;
  public static final int EDGE_INHERITANCE = 1 << 4;
  public static final int EDGE_OVERRIDE = 1 << 5;
  public static final int EDGE_TYPE_ARGUMENT = 1 << 6;
  public static final int EDGE_TEMPLATE_SPECIALIZATION = 1 << 7;
  public static final int EDGE_INCLUDE = 1 << 8;
  public static final int EDGE_IMPORT = 1 << 9;
  public static final int EDGE_BUNDLED_EDGES = 1 << 10;
  public static final int EDGE_MACRO_USAGE = 1 << 11;
  public static final int EDGE_ANNOTATION_USAGE = 1 << 12;

  public static final int LOCATION_TOKEN = 0;
  public static final int LOCATION_SCOPE = 1;
  public static final int LOCATION_QUALIFIER = 2;
  public static final int LOCATION_LOCAL_SYMBOL = 3;
  public static final int LOCATION_SIGNATURE = 4;
  public static final int LOCATION_COMMENT = 5;
  public static final int LOCATION_ERROR = 6;
  public static final int LOCATION_FULLTEXT_SEARCH = 7;
  public static final int LOCATION_SCREEN_SEARCH = 8;
  public static final int LOCATION_UNSOLVED = 9;
}
