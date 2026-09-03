package com.sourcetrail.indexer;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import sourcetrail.SourcetrailCommon.IndexerCommand;
import sourcetrail.SourcetrailCommon.IntermediateStorage;
import sourcetrail.SourcetrailCommon.StorageEdge;
import sourcetrail.SourcetrailCommon.StorageFile;
import sourcetrail.SourcetrailCommon.StorageNode;
import sourcetrail.SourcetrailCommon.StorageOccurrence;

/**
 * End-to-end test of the Java AST indexer: parse real Java source from a file,
 * emit an IntermediateStorage, and assert on the resulting proto.
 *
 * <p>These tests exercise the same code path the gRPC worker runs
 * (Main.java -> JavaIndexer.index). They are the gate for "does the emitter
 * produce a coherent graph for a representative Java file".
 */
class JavaIndexerTest {
  private final JavaIndexer indexer = new JavaIndexer();

  @TempDir
  Path workspace;

  private IntermediateStorage index(String source) throws IOException {
    return index(source, "Test.java", List.of());
  }

  private IntermediateStorage index(String source, String fileName, List<String> classPaths) throws IOException {
    Path file = workspace.resolve(fileName);
    Files.createDirectories(file.getParent());
    Files.writeString(file, source);
    return indexer.index(IndexerCommand.newBuilder()
        .setType(IndexerCommand.CommandType.JAVA)
        .setSourceFilePath(file.toString())
        .addAllClassPaths(classPaths)
        .build());
  }

  // ---- helpers ----------------------------------------------------

  private StorageNode node(IntermediateStorage s, int type, String nameContains) {
    return s.getNodesList().stream()
        .filter(n -> n.getType() == type && n.getSerializedName().contains(nameContains))
        .findFirst()
        .orElseThrow(() -> new AssertionError(
            "no node type=" + type + " containing \"" + nameContains + "\" in " + names(s)));
  }

  private static List<String> names(IntermediateStorage s) {
    return s.getNodesList().stream().map(StorageNode::getSerializedName).toList();
  }

  private static boolean hasNode(IntermediateStorage s, int type) {
    return s.getNodesList().stream().anyMatch(n -> n.getType() == type);
  }

  private static boolean hasEdge(IntermediateStorage s, long from, long to, int type) {
    return s.getEdgesList().stream()
        .anyMatch(e -> e.getSourceNodeId() == from && e.getTargetNodeId() == to && e.getType() == type);
  }

  private static boolean hasEdgeFrom(IntermediateStorage s, long from, int type) {
    return s.getEdgesList().stream().anyMatch(e -> e.getSourceNodeId() == from && e.getType() == type);
  }

  private static List<StorageEdge> edgesOf(IntermediateStorage s, int type) {
    return s.getEdgesList().stream().filter(e -> e.getType() == type).toList();
  }

  /** Location types attached to an element id, via the occurrence table. */
  private static Set<Integer> locationTypesOf(IntermediateStorage s, long elementId) {
    Set<Long> locIds = s.getOccurrencesList().stream()
        .filter(o -> o.getElementId() == elementId)
        .map(StorageOccurrence::getSourceLocationId)
        .collect(HashSet::new, Set::add, Set::addAll);
    return s.getSourceLocationsList().stream()
        .filter(l -> locIds.contains(l.getId()))
        .map(l -> (int) l.getType())
        .collect(HashSet::new, Set::add, Set::addAll);
  }

  private static boolean hasSymbol(IntermediateStorage s, long id) {
    return s.getSymbolsList().stream().anyMatch(sym -> sym.getId() == id);
  }

  private static int accessOf(IntermediateStorage s, long id) {
    return s.getComponentAccessesList().stream()
        .filter(a -> a.getNodeId() == id)
        .map(a -> (int) a.getType())
        .findFirst()
        .orElse(Kinds.ACCESS_NONE);
  }

  // ---- declarations ------------------------------------------------

  @Test
  void emits_file_and_class_node() throws IOException {
    IntermediateStorage s = index("public class Foo { }\n");

    assertEquals(1, s.getFilesCount());
    StorageFile file = s.getFiles(0);
    assertEquals("Java", file.getLanguageIdentifier());
    assertTrue(file.getIndexed());
    assertTrue(file.getComplete());

    assertTrue(hasNode(s, Kinds.NODE_FILE));
    StorageNode clazz = node(s, Kinds.NODE_CLASS, "Foo");
    assertTrue(hasSymbol(s, clazz.getId()));
    assertEquals(Kinds.ACCESS_PUBLIC, accessOf(s, clazz.getId()));
  }

  @Test
  void emits_package_node_for_declaration() throws IOException {
    IntermediateStorage s = index("package com.example;\npublic class Foo { }\n", "com/example/Foo.java", List.of());
    StorageNode pkg = node(s, Kinds.NODE_PACKAGE, "example");
    assertTrue(pkg.getSerializedName().startsWith(".\tm"), "Java names use the '.' delimiter");
  }

  @Test
  void emits_interface_enum_annotation_and_record_nodes() throws IOException {
    assertTrue(hasNode(index("interface I { void m(); }\n"), Kinds.NODE_INTERFACE));
    assertTrue(hasNode(index("enum E { A, B }\n"), Kinds.NODE_ENUM));
    assertTrue(hasNode(index("@interface A { }\n"), Kinds.NODE_ANNOTATION));
    assertTrue(hasNode(index("record Point(int x, int y) { }\n"), Kinds.NODE_CLASS));
  }

  @Test
  void emits_enum_and_enum_constant_with_member_edge() throws IOException {
    IntermediateStorage s = index("enum E { A, B }\n");
    StorageNode e = node(s, Kinds.NODE_ENUM, "E");
    StorageNode a = node(s, Kinds.NODE_ENUM_CONSTANT, "A");
    assertTrue(hasEdge(s, e.getId(), a.getId(), Kinds.EDGE_MEMBER));
  }

  // ---- member edges: the symbol tree --------------------------------

  @Test
  void a_type_owns_its_methods_and_fields_through_member_edges() throws IOException {
    IntermediateStorage s = index("public class Foo {\n  private int count;\n  void m() { }\n}\n");

    long foo = node(s, Kinds.NODE_CLASS, "Foo").getId();
    long method = node(s, Kinds.NODE_METHOD, "m").getId();
    long field = node(s, Kinds.NODE_FIELD, "count").getId();

    assertTrue(hasEdge(s, foo, method, Kinds.EDGE_MEMBER), "method must be a member of its class");
    assertTrue(hasEdge(s, foo, field, Kinds.EDGE_MEMBER), "field must be a member of its class");
  }

  @Test
  void a_nested_type_is_a_member_of_its_outer_type() throws IOException {
    IntermediateStorage s = index("public class Outer {\n  static class Inner { }\n}\n");
    long outer = node(s, Kinds.NODE_CLASS, "Outer\ts").getId();
    long inner = node(s, Kinds.NODE_CLASS, "Inner").getId();
    assertTrue(hasEdge(s, outer, inner, Kinds.EDGE_MEMBER));
  }

  @Test
  void emits_field_node_with_access_and_a_type_usage_edge() throws IOException {
    IntermediateStorage s = index("public class Foo {\n  public String name;\n}\n");
    long field = node(s, Kinds.NODE_FIELD, "name").getId();

    assertEquals(Kinds.ACCESS_PUBLIC, accessOf(s, field));
    assertTrue(hasEdge(s, field, node(s, Kinds.NODE_CLASS, "String").getId(), Kinds.EDGE_TYPE_USAGE),
        "the field's declared type must be reachable from the field");
  }

  @Test
  void emits_inheritance_edges_for_extends_and_implements() throws IOException {
    IntermediateStorage s = index("import java.util.ArrayList;\n"
        + "public class Foo extends ArrayList<String> implements Runnable {\n  public void run() { }\n}\n");
    long foo = node(s, Kinds.NODE_CLASS, "Foo\ts").getId();

    assertEquals(2, edgesOf(s, Kinds.EDGE_INHERITANCE).stream().filter(e -> e.getSourceNodeId() == foo).count());
    assertTrue(hasEdge(s, foo, node(s, Kinds.NODE_INTERFACE, "Runnable").getId(), Kinds.EDGE_INHERITANCE),
        "a reference to an interface must be NODE_INTERFACE, or it will not merge with its declaration");
  }

  @Test
  void emits_annotation_usage_edge() throws IOException {
    IntermediateStorage s = index("public class Foo {\n  @Override\n  public String toString() { return \"\"; }\n}\n");
    long method = node(s, Kinds.NODE_METHOD, "toString").getId();
    assertTrue(hasEdge(s, method, node(s, Kinds.NODE_ANNOTATION, "Override").getId(), Kinds.EDGE_ANNOTATION_USAGE));
  }

  @Test
  void emits_import_edge_from_the_file_node() throws IOException {
    IntermediateStorage s = index("import java.util.List;\npublic class Foo { }\n");
    assertTrue(hasEdgeFrom(s, Storage.FILE_ID, Kinds.EDGE_IMPORT));
  }

  @Test
  void an_imported_interface_is_not_recorded_as_a_class() throws IOException {
    // The node kind is half the merge key: importing an interface as NODE_CLASS splits it from its
    // own declaration into two nodes, silently, at merge time.
    IntermediateStorage s = index("import java.util.List;\npublic class Foo { }\n");

    assertTrue(names(s).stream()
            .filter(n -> n.contains("List"))
            .count() == 1,
        "java.util.List must appear once, not once per node kind: " + names(s));
    assertTrue(hasNode(s, Kinds.NODE_INTERFACE), "List is an interface and must be recorded as one");
  }

  @Test
  void wildcard_imports_are_skipped() throws IOException {
    IntermediateStorage s = index("import java.util.*;\npublic class Foo { }\n");
    assertTrue(edgesOf(s, Kinds.EDGE_IMPORT).isEmpty(), "a wildcard import names a package, not a symbol");
  }

  // ---- references: locations and scope -----------------------------

  @Test
  void every_reference_edge_carries_a_source_location() throws IOException {
    IntermediateStorage s = index("import java.util.List;\n"
        + "public class Foo extends Thread {\n  public void run() { System.out.println(\"x\"); }\n}\n");

    for(StorageEdge edge : s.getEdgesList()) {
      if(edge.getType() == Kinds.EDGE_MEMBER) {
        continue;    // structural, not a textual reference
      }
      assertFalse(locationTypesOf(s, edge.getId()).isEmpty(),
          "edge type " + edge.getType() + " has no occurrence; the GUI cannot navigate it");
    }
  }

  @Test
  void a_call_edge_starts_at_the_enclosing_method_not_the_file() throws IOException {
    IntermediateStorage s = index("public class A {\n  void helper() { }\n  void m() { helper(); }\n}\n");

    long caller = node(s, Kinds.NODE_METHOD, "m\ts").getId();
    List<StorageEdge> calls = edgesOf(s, Kinds.EDGE_CALL);

    assertFalse(calls.isEmpty(), "expected a call edge");
    assertTrue(calls.stream().allMatch(e -> e.getSourceNodeId() != Storage.FILE_ID),
        "calls must hang off the calling method, or the call graph is empty");
    assertTrue(calls.stream().anyMatch(e -> e.getSourceNodeId() == caller));
  }

  @Test
  void a_resolved_call_lands_on_the_declaration_node_in_the_same_file() throws IOException {
    IntermediateStorage s = index("public class A {\n  void helper() { }\n  void m() { helper(); }\n}\n");

    long helper = node(s, Kinds.NODE_METHOD, "helper").getId();
    long caller = node(s, Kinds.NODE_METHOD, "m\ts").getId();
    assertTrue(hasEdge(s, caller, helper, Kinds.EDGE_CALL),
        "declaration and call site must serialize to the same name and share one node");
  }

  @Test
  void an_unresolvable_call_is_marked_unsolved() throws IOException {
    IntermediateStorage s = index("public class A {\n  void m() { totallyUnknown(); }\n}\n");

    StorageEdge call = edgesOf(s, Kinds.EDGE_CALL).stream().findFirst().orElseThrow();
    assertTrue(locationTypesOf(s, call.getId()).contains(Kinds.LOCATION_UNSOLVED),
        "an unresolved reference must say so rather than look like a confident answer");
  }

  @Test
  void emits_override_edge_for_an_implemented_interface_method() throws IOException {
    IntermediateStorage s = index("public class A implements Runnable {\n  public void run() { }\n}\n");
    long run = node(s, Kinds.NODE_METHOD, "run").getId();
    assertTrue(hasEdgeFrom(s, run, Kinds.EDGE_OVERRIDE));
  }

  @Test
  void reading_a_field_emits_a_usage_edge_to_it() throws IOException {
    IntermediateStorage s = index("public class A {\n  int count;\n  int m() { return count; }\n}\n");

    long field = node(s, Kinds.NODE_FIELD, "count").getId();
    long method = node(s, Kinds.NODE_METHOD, "m\ts").getId();
    assertTrue(hasEdge(s, method, field, Kinds.EDGE_USAGE));
  }

  @Test
  void a_bare_local_variable_read_does_not_fabricate_a_class_node() throws IOException {
    IntermediateStorage s = index("package com.example;\n"
        + "public class A {\n  void m() { int local = 1; int other = local; }\n}\n", "com/example/A.java", List.of());

    assertTrue(names(s).stream().noneMatch(n -> n.contains("com\ts\tp\tnexample\ts\tp\tnlocal")),
        "a local variable read must not become a class in the current package");
  }

  // ---- local symbols ------------------------------------------------

  @Test
  void parameters_and_locals_become_local_symbols_with_locations() throws IOException {
    IntermediateStorage s = index("public class A {\n  void m(int x) { int y = x; }\n}\n");

    assertEquals(2, s.getLocalSymbolsCount(), "one symbol per declaration: the parameter and the local");
    for(var local : s.getLocalSymbolsList()) {
      assertTrue(local.getName().startsWith("Test.java<"), "local symbol names follow the C++ file<line:col> form");
      assertTrue(locationTypesOf(s, local.getId()).contains(Kinds.LOCATION_LOCAL_SYMBOL));
    }
  }

  @Test
  void every_use_of_one_variable_shares_a_single_local_symbol() throws IOException {
    IntermediateStorage s = index("public class A {\n  void m(int x) { int a = x; int b = x; int c = x; }\n}\n");

    long param = s.getLocalSymbolsList().stream()
        .filter(l -> l.getName().contains("<2:10>"))
        .findFirst()
        .orElseThrow()
        .getId();

    long occurrences = s.getOccurrencesList().stream().filter(o -> o.getElementId() == param).count();
    assertEquals(4, occurrences, "the declaration plus its three reads");
  }

  // ---- failure handling ---------------------------------------------

  @Test
  void a_parse_error_yields_an_error_row_and_an_incomplete_file() throws IOException {
    IntermediateStorage s = index("public class Broken { this is not java (((\n");

    assertTrue(s.getErrorsCount() > 0, "a parse failure must be reported, not swallowed");
    assertFalse(s.getFiles(0).getComplete(), "an unparsed file must not look fully indexed");
  }

  @Test
  void a_non_java_command_yields_an_empty_storage() {
    IntermediateStorage s = indexer.index(IndexerCommand.newBuilder()
        .setType(IndexerCommand.CommandType.CXX)
        .setSourceFilePath("/tmp/a.cpp")
        .build());
    assertEquals(1, s.getNextId());
    assertEquals(0, s.getNodesCount());
  }

  // ---- ids -----------------------------------------------------------

  @Test
  void ids_are_unique_across_every_table_and_below_next_id() throws IOException {
    IntermediateStorage s = index("import java.util.List;\n"
        + "public class A {\n  int f;\n  void m(int x) { int y = x; toString(); }\n}\n");

    Set<Long> seen = new HashSet<>();
    for(long id : allIds(s)) {
      assertTrue(id < s.getNextId(), "id " + id + " is not below nextId " + s.getNextId());
      assertTrue(seen.add(id), "id " + id + " was allocated twice");
    }
    assertNotEquals(0, seen.size());
  }

  private static List<Long> allIds(IntermediateStorage s) {
    List<Long> ids = new java.util.ArrayList<>();
    s.getNodesList().forEach(n -> ids.add(n.getId()));
    s.getEdgesList().forEach(e -> ids.add(e.getId()));
    s.getLocalSymbolsList().forEach(l -> ids.add(l.getId()));
    s.getSourceLocationsList().forEach(l -> ids.add(l.getId()));
    s.getErrorsList().forEach(e -> ids.add(e.getId()));
    return ids;
  }

  // ---- cross-file resolution (the point of the symbol solver) --------

  @Test
  void a_call_into_another_file_resolves_through_the_classpath() throws IOException {
    Files.writeString(workspace.resolve("B.java"), "public class B {\n  public void n() { }\n}\n");

    IntermediateStorage s = index("public class A {\n  void m(B b) { b.n(); }\n}\n",
        "A.java", List.of(workspace.toString()));

    long caller = node(s, Kinds.NODE_METHOD, "m\ts").getId();
    StorageNode target = node(s, Kinds.NODE_METHOD, "B\ts\tp\tnn\ts");

    assertTrue(hasEdge(s, caller, target.getId(), Kinds.EDGE_CALL),
        "with B's source root on the classpath, b.n() must resolve to B.n; got " + names(s));

    StorageEdge call = edgesOf(s, Kinds.EDGE_CALL).stream().findFirst().orElseThrow();
    assertFalse(locationTypesOf(s, call.getId()).contains(Kinds.LOCATION_UNSOLVED));
  }
}
