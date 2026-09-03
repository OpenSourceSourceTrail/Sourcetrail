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

  private TestStorage index(String source) throws IOException {
    return index(source, "Test.java", List.of());
  }

  private TestStorage index(String source, String fileName, List<String> classPaths) throws IOException {
    Path file = workspace.resolve(fileName);
    Files.createDirectories(file.getParent());
    Files.writeString(file, source);
    return TestStorage.create(indexer.index(IndexerCommand.newBuilder()
        .setType(IndexerCommand.CommandType.JAVA)
        .setSourceFilePath(file.toString())
        .addAllClassPaths(classPaths)
        .build()));
  }

  // ---- declarations ------------------------------------------------

  @Test
  void emits_file_and_class_node() throws IOException {
    TestStorage s = index("public class Foo { }\n");

    assertEquals(1, s.proto().getFilesCount());
    StorageFile file = s.proto().getFiles(0);
    assertEquals("Java", file.getLanguageIdentifier());
    assertTrue(file.getIndexed());
    assertTrue(file.getComplete());

    assertTrue(s.hasNode(Kinds.NODE_FILE));
    StorageNode clazz = s.node(Kinds.NODE_CLASS, "Foo");
    assertTrue(s.hasSymbol(clazz.getId()));
    assertEquals(Kinds.ACCESS_PUBLIC, s.accessOf(clazz.getId()));
  }

  @Test
  void emits_package_node_for_declaration() throws IOException {
    TestStorage s = index("package com.example;\npublic class Foo { }\n", "com/example/Foo.java", List.of());
    StorageNode pkg = s.node(Kinds.NODE_PACKAGE, "example");
    assertTrue(pkg.getSerializedName().startsWith(".\tm"), "Java names use the '.' delimiter");
  }

  @Test
  void emits_interface_enum_annotation_and_record_nodes() throws IOException {
    assertTrue(index("interface I { void m(); }\n").hasNode(Kinds.NODE_INTERFACE));
    assertTrue(index("enum E { A, B }\n").hasNode(Kinds.NODE_ENUM));
    assertTrue(index("@interface A { }\n").hasNode(Kinds.NODE_ANNOTATION));
    assertTrue(index("record Point(int x, int y) { }\n").hasNode(Kinds.NODE_CLASS));
  }

  @Test
  void emits_enum_and_enum_constant_with_member_edge() throws IOException {
    TestStorage s = index("enum E { A, B }\n");
    StorageNode e = s.node(Kinds.NODE_ENUM, "E");
    StorageNode a = s.node(Kinds.NODE_ENUM_CONSTANT, "A");
    assertTrue(s.hasEdge(e.getId(), a.getId(), Kinds.EDGE_MEMBER));
  }

  // ---- member edges: the symbol tree --------------------------------

  @Test
  void a_type_owns_its_methods_and_fields_through_member_edges() throws IOException {
    TestStorage s = index("public class Foo {\n  private int count;\n  void m() { }\n}\n");

    long foo = s.node(Kinds.NODE_CLASS, "Foo").getId();
    long method = s.node(Kinds.NODE_METHOD, "m").getId();
    long field = s.node(Kinds.NODE_FIELD, "count").getId();

    assertTrue(s.hasEdge(foo, method, Kinds.EDGE_MEMBER), "method must be a member of its class");
    assertTrue(s.hasEdge(foo, field, Kinds.EDGE_MEMBER), "field must be a member of its class");
  }

  @Test
  void a_nested_type_is_a_member_of_its_outer_type() throws IOException {
    TestStorage s = index("public class Outer {\n  static class Inner { }\n}\n");
    long outer = s.node(Kinds.NODE_CLASS, "Outer\ts").getId();
    long inner = s.node(Kinds.NODE_CLASS, "Inner").getId();
    assertTrue(s.hasEdge(outer, inner, Kinds.EDGE_MEMBER));
  }

  @Test
  void emits_field_node_with_access_and_a_type_usage_edge() throws IOException {
    TestStorage s = index("public class Foo {\n  public String name;\n}\n");
    long field = s.node(Kinds.NODE_FIELD, "name").getId();

    assertEquals(Kinds.ACCESS_PUBLIC, s.accessOf(field));
    assertTrue(s.hasEdge(field, s.node(Kinds.NODE_CLASS, "String").getId(), Kinds.EDGE_TYPE_USAGE),
        "the field's declared type must be reachable from the field");
  }

  @Test
  void emits_inheritance_edges_for_extends_and_implements() throws IOException {
    TestStorage s = index("import java.util.ArrayList;\n"
        + "public class Foo extends ArrayList<String> implements Runnable {\n  public void run() { }\n}\n");
    long foo = s.node(Kinds.NODE_CLASS, "Foo\ts").getId();

    assertEquals(2, s.edgesOf(Kinds.EDGE_INHERITANCE).stream().filter(e -> e.getSourceNodeId() == foo).count());
    assertTrue(s.hasEdge(foo, s.node(Kinds.NODE_INTERFACE, "Runnable").getId(), Kinds.EDGE_INHERITANCE),
        "a reference to an interface must be NODE_INTERFACE, or it will not merge with its declaration");
  }

  @Test
  void emits_annotation_usage_edge() throws IOException {
    TestStorage s = index("public class Foo {\n  @Override\n  public String toString() { return \"\"; }\n}\n");
    long method = s.node(Kinds.NODE_METHOD, "toString").getId();
    assertTrue(s.hasEdge(method, s.node(Kinds.NODE_ANNOTATION, "Override").getId(), Kinds.EDGE_ANNOTATION_USAGE));
  }

  @Test
  void emits_import_edge_from_the_file_node() throws IOException {
    TestStorage s = index("import java.util.List;\npublic class Foo { }\n");
    assertTrue(s.hasEdgeFrom(Storage.FILE_ID, Kinds.EDGE_IMPORT));
  }

  @Test
  void an_imported_interface_is_not_recorded_as_a_class() throws IOException {
    // The node kind is half the merge key: importing an interface as NODE_CLASS splits it from its
    // own declaration into two nodes, silently, at merge time.
    TestStorage s = index("import java.util.List;\npublic class Foo { }\n");

    assertTrue(s.names().stream()
            .filter(n -> n.contains("List"))
            .count() == 1,
        "java.util.List must appear once, not once per node kind: " + s.names());
    assertTrue(s.hasNode(Kinds.NODE_INTERFACE), "List is an interface and must be recorded as one");
  }

  @Test
  void wildcard_imports_are_skipped() throws IOException {
    TestStorage s = index("import java.util.*;\npublic class Foo { }\n");
    assertTrue(s.edgesOf(Kinds.EDGE_IMPORT).isEmpty(), "a wildcard import names a package, not a symbol");
  }

  // ---- references: locations and scope -----------------------------

  @Test
  void every_reference_edge_carries_a_source_location() throws IOException {
    TestStorage s = index("import java.util.List;\n"
        + "public class Foo extends Thread {\n  public void run() { System.out.println(\"x\"); }\n}\n");

    for(StorageEdge edge : s.proto().getEdgesList()) {
      if(edge.getType() == Kinds.EDGE_MEMBER) {
        continue;    // structural, not a textual reference
      }
      assertFalse(s.locationTypesOf(edge.getId()).isEmpty(),
          "edge type " + edge.getType() + " has no occurrence; the GUI cannot navigate it");
    }
  }

  @Test
  void a_call_edge_starts_at_the_enclosing_method_not_the_file() throws IOException {
    TestStorage s = index("public class A {\n  void helper() { }\n  void m() { helper(); }\n}\n");

    long caller = s.node(Kinds.NODE_METHOD, "m\ts").getId();
    List<StorageEdge> calls = s.edgesOf(Kinds.EDGE_CALL);

    assertFalse(calls.isEmpty(), "expected a call edge");
    assertTrue(calls.stream().allMatch(e -> e.getSourceNodeId() != Storage.FILE_ID),
        "calls must hang off the calling method, or the call graph is empty");
    assertTrue(calls.stream().anyMatch(e -> e.getSourceNodeId() == caller));
  }

  @Test
  void a_resolved_call_lands_on_the_declaration_node_in_the_same_file() throws IOException {
    TestStorage s = index("public class A {\n  void helper() { }\n  void m() { helper(); }\n}\n");

    long helper = s.node(Kinds.NODE_METHOD, "helper").getId();
    long caller = s.node(Kinds.NODE_METHOD, "m\ts").getId();
    assertTrue(s.hasEdge(caller, helper, Kinds.EDGE_CALL),
        "declaration and call site must serialize to the same name and share one node");
  }

  @Test
  void an_unresolvable_call_is_marked_unsolved() throws IOException {
    TestStorage s = index("public class A {\n  void m() { totallyUnknown(); }\n}\n");

    StorageEdge call = s.edgesOf(Kinds.EDGE_CALL).stream().findFirst().orElseThrow();
    assertTrue(s.locationTypesOf(call.getId()).contains(Kinds.LOCATION_UNSOLVED),
        "an unresolved reference must say so rather than look like a confident answer");
  }

  @Test
  void emits_override_edge_for_an_implemented_interface_method() throws IOException {
    TestStorage s = index("public class A implements Runnable {\n  public void run() { }\n}\n");
    long run = s.node(Kinds.NODE_METHOD, "run").getId();
    assertTrue(s.hasEdgeFrom(run, Kinds.EDGE_OVERRIDE));
  }

  @Test
  void reading_a_field_emits_a_usage_edge_to_it() throws IOException {
    TestStorage s = index("public class A {\n  int count;\n  int m() { return count; }\n}\n");

    long field = s.node(Kinds.NODE_FIELD, "count").getId();
    long method = s.node(Kinds.NODE_METHOD, "m\ts").getId();
    assertTrue(s.hasEdge(method, field, Kinds.EDGE_USAGE));
  }

  @Test
  void a_bare_local_variable_read_does_not_fabricate_a_class_node() throws IOException {
    TestStorage s = index("package com.example;\n"
        + "public class A {\n  void m() { int local = 1; int other = local; }\n}\n", "com/example/A.java", List.of());

    assertTrue(s.names().stream().noneMatch(n -> n.contains("com\ts\tp\tnexample\ts\tp\tnlocal")),
        "a local variable read must not become a class in the current package");
  }

  // ---- local symbols ------------------------------------------------

  @Test
  void parameters_and_locals_become_local_symbols_with_locations() throws IOException {
    TestStorage s = index("public class A {\n  void m(int x) { int y = x; }\n}\n");

    assertEquals(2, s.proto().getLocalSymbolsCount(), "one symbol per declaration: the parameter and the local");
    for(var local : s.proto().getLocalSymbolsList()) {
      assertTrue(local.getName().startsWith("Test.java<"), "local symbol names follow the C++ file<line:col> form");
      assertTrue(s.locationTypesOf(local.getId()).contains(Kinds.LOCATION_LOCAL_SYMBOL));
    }
  }

  @Test
  void every_use_of_one_variable_shares_a_single_local_symbol() throws IOException {
    TestStorage s = index("public class A {\n  void m(int x) { int a = x; int b = x; int c = x; }\n}\n");

    long param = s.proto().getLocalSymbolsList().stream()
        .filter(l -> l.getName().contains("<2:10>"))
        .findFirst()
        .orElseThrow()
        .getId();

    long occurrences = s.proto().getOccurrencesList().stream().filter(o -> o.getElementId() == param).count();
    assertEquals(4, occurrences, "the declaration plus its three reads");
  }

  // ---- failure handling ---------------------------------------------

  @Test
  void a_parse_error_yields_an_error_row_and_an_incomplete_file() throws IOException {
    TestStorage s = index("public class Broken { this is not java (((\n");

    assertTrue(s.proto().getErrorsCount() > 0, "a parse failure must be reported, not swallowed");
    assertFalse(s.proto().getFiles(0).getComplete(), "an unparsed file must not look fully indexed");
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

  // ---- type parameters ------------------------------------------------

  /**
   * A type parameter is a symbol of its own. Emitting it as a local variable left every use of
   * {@code T} to be resolved lexically, which fabricated a class node named {@code T} in the
   * current package -- the type-level twin of
   * {@link #a_bare_local_variable_read_does_not_fabricate_a_class_node}.
   */
  @Test
  void a_type_parameter_is_a_node_its_uses_resolve_to() throws IOException {
    TestStorage s = index("package com.example;\n"
        + "public class Box<T> {\n"
        + "  private T value;\n"
        + "}\n", "com/example/Box.java", List.of());

    long parameter = s.node(Kinds.NODE_TYPE_PARAMETER, "T").getId();
    long box = s.node(Kinds.NODE_CLASS, "Box\ts").getId();
    long value = s.node(Kinds.NODE_FIELD, "value").getId();

    assertEquals(Kinds.ACCESS_TYPE_PARAMETER, s.accessOf(parameter));
    assertTrue(s.hasEdge(box, parameter, Kinds.EDGE_MEMBER), "T belongs to Box");
    assertTrue(s.hasEdge(value, parameter, Kinds.EDGE_TYPE_USAGE),
        "the field's type must reach the parameter, not a fabricated class");
    assertTrue(s.names().stream().noneMatch(n -> n.contains("example\ts\tp\tnT")),
        "a use of T must not become a class in the current package: " + s.names());
  }

  /** Method-level parameters carry their method's signature in the container name. */
  @Test
  void a_method_type_parameter_is_scoped_to_its_method() throws IOException {
    TestStorage s = index("public class Util {\n"
        + "  static <U> U pick(U a, String b) { return a; }\n"
        + "}\n");

    assertTrue(s.typeParameters.contains("Util.pick(U, java.lang.String).U <2:11 2:11>"),
        "type parameters: " + s.typeParameters);
    assertTrue(s.typeUses.contains(
        "U Util.pick(U, java.lang.String) -> Util.pick(U, java.lang.String).U <2:21 2:21>"),
        "the parameter's use must reach its declaration: " + s.typeUses);
  }

  /**
   * Members of an anonymous class belong to it, not to the class it sits inside. findAncestor on
   * TypeDeclaration walks straight past an anonymous body, which used to attach them to the
   * enclosing named type -- so the graph said Outer owned a method whose own name said otherwise.
   */
  @Test
  void anonymous_class_members_belong_to_the_anonymous_class() throws IOException {
    TestStorage s = index("public class Outer {\n"
        + "  Object make() {\n"
        + "    return new Object() {\n"
        + "      int x = 1;\n"
        + "      void helper() { int v = x; }\n"
        + "    };\n"
        + "  }\n"
        + "}\n");

    long outer = s.node(Kinds.NODE_CLASS, "Outer\ts").getId();
    long anonymous = s.node(Kinds.NODE_CLASS, "anonymous class").getId();
    long field = s.node(Kinds.NODE_FIELD, "x").getId();
    long helper = s.node(Kinds.NODE_METHOD, "helper").getId();

    assertTrue(s.hasEdge(anonymous, field, Kinds.EDGE_MEMBER), "x belongs to the anonymous class");
    assertTrue(s.hasEdge(anonymous, helper, Kinds.EDGE_MEMBER), "helper belongs to it too");
    assertFalse(s.hasEdge(outer, field, Kinds.EDGE_MEMBER), "and not to Outer");
    assertEquals(1, s.fields.size(),
        "the read of x must resolve to the declared field, not add a second: " + s.fields);
  }

  // ---- record components ---------------------------------------------

  /**
   * A record component declares a private final field (JLS 8.10.3), but JavaParser models it as a
   * Parameter -- so without an explicit emission it falls through visit(Parameter) and is recorded
   * as a local variable, leaving the record with no members at all.
   */
  @Test
  void record_components_are_fields_of_the_record() throws IOException {
    TestStorage s = index("record Point(int x, int y) { }\n");

    long point = s.node(Kinds.NODE_CLASS, "Point").getId();
    long x = s.node(Kinds.NODE_FIELD, "x").getId();
    long y = s.node(Kinds.NODE_FIELD, "y").getId();

    assertTrue(s.hasEdge(point, x, Kinds.EDGE_MEMBER), "x must be a member of Point");
    assertTrue(s.hasEdge(point, y, Kinds.EDGE_MEMBER), "y must be a member of Point");
    assertEquals(Kinds.ACCESS_PRIVATE, s.accessOf(x));
    assertTrue(s.locationTypesOf(x).contains(Kinds.LOCATION_TOKEN),
        "the component needs a token location or the GUI cannot navigate to it");
    assertTrue(s.proto().getLocalSymbolsList().isEmpty(),
        "components are fields, not locals: " + s.proto().getLocalSymbolsList());
  }

  @Test
  void a_component_read_in_the_compact_constructor_merges_onto_its_field() throws IOException {
    // Declaration and reference must serialize identically, or the record ends up with two
    // "x" nodes -- one declared, one fabricated by the read.
    TestStorage s = index("record Point(int x, int y) {\n"
        + "  Point {\n"
        + "    if(x < 0) { throw new IllegalArgumentException(); }\n"
        + "  }\n"
        + "}\n");

    assertEquals(2, s.fields.size(), "one node per component, not one per mention: " + s.fields);
  }

  // ---- anonymous classes --------------------------------------------

  /**
   * JavaParser's symbol solver names anonymous classes {@code Anonymous-<random UUID>}. The engine
   * dedups nodes on (type, serializedName) at merge time, so passing that through means an
   * anonymous class and everything declared in it gets a brand-new identity on every index run and
   * never merges with itself. JavaCollector replaces it with the C++ indexer's positional
   * convention; this pins both halves of that.
   */
  @Test
  void anonymous_class_names_are_stable_and_carry_no_random_id() throws IOException {
    String source = "public class Outer {\n"
        + "  Runnable make() {\n"
        + "    return new Runnable() {\n"
        + "      public void run() { }\n"
        + "    };\n"
        + "  }\n"
        + "}\n";

    TestStorage first = index(source);
    TestStorage second = index(source);

    assertEquals(first.names(), second.names(),
        "the same source must serialize to the same names on every run, or nothing merges");
    assertTrue(first.names().stream().noneMatch(n -> n.contains("Anonymous-")),
        "no name may carry the solver's random anonymous id: " + first.names());
    assertTrue(first.methods.contains(
        "public void Outer.anonymous class (Test.java<3:12>).run() <4:7 <4:19 4:21> 4:27>"),
        "anonymous class members are named by source position: " + first.methods);
  }

  @Test
  void ids_are_unique_across_every_table_and_below_next_id() throws IOException {
    TestStorage s = index("import java.util.List;\n"
        + "public class A {\n  int f;\n  void m(int x) { int y = x; toString(); }\n}\n");

    Set<Long> seen = new HashSet<>();
    for(long id : allIds(s.proto())) {
      assertTrue(id < s.proto().getNextId(), "id " + id + " is not below nextId " + s.proto().getNextId());
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

    TestStorage s = index("public class A {\n  void m(B b) { b.n(); }\n}\n",
        "A.java", List.of(workspace.toString()));

    long caller = s.node(Kinds.NODE_METHOD, "m\ts").getId();
    StorageNode target = s.node(Kinds.NODE_METHOD, "B\ts\tp\tnn\ts");

    assertTrue(s.hasEdge(caller, target.getId(), Kinds.EDGE_CALL),
        "with B's source root on the classpath, b.n() must resolve to B.n; got " + s.names());

    StorageEdge call = s.edgesOf(Kinds.EDGE_CALL).stream().findFirst().orElseThrow();
    assertFalse(s.locationTypesOf(call.getId()).contains(Kinds.LOCATION_UNSOLVED));
  }
}
