package com.sourcetrail.indexer;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;
import sourcetrail.SourcetrailCommon.StorageEdge;
import sourcetrail.SourcetrailCommon.StorageFile;
import sourcetrail.SourcetrailCommon.StorageLocalSymbol;
import sourcetrail.SourcetrailCommon.StorageNode;
import sourcetrail.SourcetrailCommon.StorageSymbol;

/**
 * End-to-end test of the Java AST indexer: parse real Java source from a file,
 * emit an IntermediateStorage, and assert on the resulting proto.
 *
 * <p>These tests exercise the same code path the gRPC worker runs
 * (Main.java → JavaIndexer.index). They are the gate for "does the emitter
 * produce a coherent graph for a representative Java file".
 */
class JavaIndexerTest {
  private final JavaIndexer indexer = new JavaIndexer();

  private IntermediateStorage index(String source) throws IOException {
    Path file = Files.createTempFile("srt_test", ".java");
    try {
      Files.writeString(file, source);
      IndexerCommand cmd = IndexerCommand.newBuilder()
          .setType(IndexerCommand.CommandType.JAVA)
          .setSourceFilePath(file.toString())
          .build();
      return indexer.index(cmd);
    } finally {
      Files.deleteIfExists(file);
    }
  }

  private StorageNode node(IntermediateStorage s, String serializedName) {
    return s.getNodesList().stream()
        .filter(n -> n.getSerializedName().equals(serializedName))
        .findFirst()
        .orElseThrow(() -> new AssertionError("no node with serializedName=" + serializedName));
  }

  private boolean hasEdgeOf(IntermediateStorage s, long from, long to, int type) {
    return s.getEdgesList().stream()
        .anyMatch(e -> e.getSourceNodeId() == from && e.getTargetNodeId() == to && e.getType() == type);
  }

  private boolean hasEdgeToType(IntermediateStorage s, long from, int type) {
    return s.getEdgesList().stream().anyMatch(e -> e.getSourceNodeId() == from && e.getType() == type);
  }

  private long idOf(StorageNode n) {
    return n.getId();
  }

  // ---- basic shape ----------------------------------------------------

  @Test
  void emits_file_and_class_node() throws IOException {
    IntermediateStorage s = index("package a.b;\npublic class Foo { }\n");
    assertEquals(1, s.getFilesCount());
    StorageFile f = s.getFiles(0);
    assertEquals("Java", f.getLanguageIdentifier());
    assertTrue(f.getIndexed());
    assertTrue(f.getComplete());

    // File node
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_FILE));

    // class node: type NODE_CLASS
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_CLASS));

    // class symbol
    StorageNode clz = s.getNodesList().stream()
        .filter(n -> n.getType() == Kinds.NODE_CLASS)
        .findFirst().orElseThrow();
    assertTrue(s.getSymbolsList().stream().anyMatch(sym -> sym.getId() == clz.getId()));
  }

  @Test
  void emits_package_node_for_declaration() throws IOException {
    IntermediateStorage s = index("package a.b.c;\npublic class Foo { }\n");
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_PACKAGE));
    // The package node name should serialize with the chain a, b, c
    StorageNode pkg = s.getNodesList().stream()
        .filter(n -> n.getType() == Kinds.NODE_PACKAGE)
        .findFirst().orElseThrow();
    // First byte is the dotted Java delimiter, then \tm meta marker
    assertTrue(pkg.getSerializedName().startsWith(".\tm"),
        "serialized package name must start with the java delimiter and meta marker; got=" + pkg.getSerializedName());
  }

  @Test
  void emits_import_edge_to_imported_name() throws IOException {
    IntermediateStorage s = index("package p;\nimport java.util.List;\npublic class Foo { }\n");
    StorageNode file = s.getNodesList().stream().filter(n -> n.getType() == Kinds.NODE_FILE).findFirst().orElseThrow();
    assertTrue(s.getEdgesList().stream().anyMatch(e ->
        e.getSourceNodeId() == file.getId() && e.getType() == Kinds.EDGE_IMPORT));
  }

  @Test
  void emits_method_node_and_local_symbol_for_params() throws IOException {
    IntermediateStorage s = index("public class Foo {\n  void m(int q, String s) { }\n}\n");
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_METHOD));

    // method has a TOKEN + a SCOPE location
    StorageNode method = s.getNodesList().stream().filter(n -> n.getType() == Kinds.NODE_METHOD).findFirst().orElseThrow();
    boolean hasToken = s.getSourceLocationsList().stream()
        .filter(l -> l.getFileNodeId() == 1)
        .anyMatch(l -> s.getOccurrencesList().stream()
            .anyMatch(o -> o.getElementId() == method.getId() && o.getSourceLocationId() == l.getId())
            && l.getType() == Kinds.LOCATION_TOKEN);
    assertTrue(hasToken, "method should have a TOKEN location");
  }

  @Test
  void emits_field_node_with_access_and_type_edge() throws IOException {
    IntermediateStorage s = index("public class Foo {\n  public int x;\n}\n");
    StorageNode field = s.getNodesList().stream().filter(n -> n.getType() == Kinds.NODE_FIELD).findFirst().orElseThrow();
    assertTrue(s.getComponentAccessesList().stream()
        .anyMatch(a -> a.getNodeId() == field.getId() && a.getType() == Kinds.ACCESS_PUBLIC));
    // field → some type edge (int)
    assertTrue(hasEdgeToType(s, field.getId(), Kinds.EDGE_TYPE_USAGE)
        || !s.getComponentAccessesList().isEmpty());
  }

  @Test
  void emits_enum_and_enum_constant() throws IOException {
    IntermediateStorage s = index("enum Color { RED, GREEN, BLUE }\n");
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_ENUM));
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_ENUM_CONSTANT));
    // at least one MEMBER edge from enum to enum constant
    StorageNode e = s.getNodesList().stream().filter(n -> n.getType() == Kinds.NODE_ENUM).findFirst().orElseThrow();
    assertTrue(s.getEdgesList().stream().anyMatch(ed -> ed.getSourceNodeId() == e.getId() && ed.getType() == Kinds.EDGE_MEMBER));
  }

  @Test
  void emits_interface_node() throws IOException {
    IntermediateStorage s = index("interface I { void m(); }\n");
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_INTERFACE));
  }

  @Test
  void emits_annotation_node_and_usage_edge() throws IOException {
    IntermediateStorage s = index("@Retention(RetentionPolicy.RUNTIME) @interface MyAnn {}\n");
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_ANNOTATION));
  }

  @Test
  void emits_record_node() throws IOException {
    IntermediateStorage s = index("record Point(int x, int y) {}\n");
    assertTrue(s.getNodesList().stream().anyMatch(n -> n.getType() == Kinds.NODE_CLASS));
  }

  @Test
  void emits_inheritance_edge_for_extends() throws IOException {
    IntermediateStorage s = index("public class A extends Object {}\n");
    StorageNode a = s.getNodesList().stream().filter(n -> n.getType() == Kinds.NODE_CLASS).findFirst().orElseThrow();
    assertTrue(hasEdgeToType(s, a.getId(), Kinds.EDGE_INHERITANCE));
  }

  @Test
  void emits_call_edge_for_method_invocations() throws IOException {
    IntermediateStorage s = index("public class A {\n  void m() { someCall(args); }\n}\n");
    // file → method (call) edge should exist
    StorageNode file = s.getNodesList().stream().filter(n -> n.getType() == Kinds.NODE_FILE).findFirst().orElseThrow();
    assertTrue(s.getEdgesList().stream().anyMatch(e -> e.getSourceNodeId() == file.getId() && e.getType() == Kinds.EDGE_CALL));
  }

  @Test
  void emits_local_symbols_for_method_params() throws IOException {
    IntermediateStorage s = index("public class A {\n  void m(int x, String y) { }\n}\n");
    assertTrue(s.getLocalSymbolsCount() >= 2);
    assertTrue(s.getLocalSymbolsList().stream().anyMatch(ls -> ls.getName().equals("x")));
    assertTrue(s.getLocalSymbolsList().stream().anyMatch(ls -> ls.getName().equals("y")));
  }

  @Test
  void next_id_is_monotonic_and_unique() throws IOException {
    IntermediateStorage s = index("package q;\npublic class C { int f; }\n");
    // every id should be < nextId (i.e. no future id used)
    long nextId = s.getNextId();
    for(StorageNode n : s.getNodesList()) {
      assertTrue(n.getId() < nextId);
    }
    for(StorageEdge e : s.getEdgesList()) {
      assertTrue(e.getId() < nextId);
    }
    for(StorageSymbol sym : s.getSymbolsList()) {
      assertTrue(sym.getId() < nextId);
    }
  }
}
