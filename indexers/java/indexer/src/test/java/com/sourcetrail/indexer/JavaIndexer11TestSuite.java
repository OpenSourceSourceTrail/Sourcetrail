package com.sourcetrail.indexer;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.Test;

/**
 * Language-feature coverage for the Java 9–11 span, mirroring the style of
 * {@code tests/integration/lib_cxx/CxxParser11TestSuite.cpp}: one {@code @Test} per
 * language feature, a Javadoc linking the JLS chapter and the JEP, snippet in, bin assertion out.
 *
 * <p>Features covered: private interface method (JEP 213), {@code var} local-variable type
 * inference (JEP 286), {@code var} in lambda parameters (JEP 323), diamond operator with an
 * anonymous class (JEP 213), try-with-resources over an effectively-final variable (JEP 213),
 * anonymous inner class with its own members, and a generic class with a bounded type parameter.
 *
 * <p>Deliberately excluded: {@code module-info.java} (JEP 261) — a different compilation-unit
 * shape that {@code JavaCollector} does not walk (no {@code NODE_MODULE} emission today).
 */
class JavaIndexer11TestSuite extends JavaStdTestSuite {
  @Override
  protected String standard() { return "11"; }

  // ---- JEP 213 / JLS 9.6.1 -- private interface method ---------------

  /**
   * Private methods in interfaces (JEP 213, Java 9).
   *
   * <p>JLS §9.4: Interface member declarations.
   * JEP: <a href="https://openjdk.org/jeps/213">JEP 213 – Milling Project Coin</a>
   */
  @Test
  void private_interface_method() throws Exception {
    TestStorage s = index(
        "interface Greeter {\n"
        + "  default String greet() { return helper(); }\n"
        + "  private String helper() { return \"hi\"; }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.methods.contains(
        "public java.lang.String Greeter.greet() <2:3 <2:18 2:22> 2:45>"),
        "greet() should appear in methods: " + s.methods);
    assertTrue(s.methods.contains(
        "private java.lang.String Greeter.helper() <3:3 <3:18 3:23> 3:42>"),
        "private helper() should appear in methods: " + s.methods);
    assertTrue(s.calls.contains(
        "java.lang.String Greeter.greet() -> java.lang.String Greeter.helper() <2:35 2:40>"),
        "call from greet() to helper(): " + s.calls);
  }

  // ---- JEP 286 / JLS 14.4 -- var local-variable type inference --------

  /**
   * Local-variable type inference via {@code var} (JEP 286, Java 10).
   *
   * <p>JLS §14.4: Local variable declaration statements.
   * JEP: <a href="https://openjdk.org/jeps/286">JEP 286 – Local-Variable Type Inference</a>
   */
  @Test
  void var_local_variable_type_inference() throws Exception {
    TestStorage s = index(
        "public class VarDemo {\n"
        + "  void m() {\n"
        + "    var count = 42;\n"
        + "    var name = \"hello\";\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    // var locals are still emitted as local symbols; the inferred type is not recorded
    // in the localSymbols bin (no type-use edge from a local symbol).
    assertTrue(s.localSymbols.contains("Test.java<3:9> <3:9 3:13>"),
        "var count should produce a local symbol at col 9: " + s.localSymbols);
    assertTrue(s.localSymbols.contains("Test.java<4:9> <4:9 4:12>"),
        "var name should produce a local symbol at col 9: " + s.localSymbols);
    assertTrue(s.classes.contains("public VarDemo <1:1 <1:14 1:20> 6:1>"),
        "VarDemo class: " + s.classes);
  }

  // ---- JEP 323 / JLS 15.27 -- var in lambda parameters ---------------

  /**
   * {@code var} in lambda parameters (JEP 323, Java 11).
   *
   * <p>JLS §15.27.1: Lambda parameters.
   * JEP: <a href="https://openjdk.org/jeps/323">JEP 323 – Local-Variable Syntax for Lambda Parameters</a>
   */
  @Test
  void var_in_lambda_parameters() throws Exception {
    TestStorage s = index(
        "import java.util.function.BiFunction;\n"
        + "public class LambdaVar {\n"
        + "  void m() {\n"
        + "    BiFunction<Integer,Integer,Integer> add = (var x, var y) -> x + y;\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    // var lambda parameters produce local symbols just like explicit-type parameters
    assertTrue(s.localSymbols.contains("Test.java<4:41> <4:41 4:43>"),
        "var x parameter should be a local symbol: " + s.localSymbols);
    assertTrue(s.methods.contains("default void LambdaVar.m() <3:3 <3:8 3:8> 5:3>"),
        "enclosing method m(): " + s.methods);
  }

  // ---- JEP 213 / JLS 15.9.5 -- diamond with anonymous class ----------

  /**
   * Diamond operator with anonymous class instantiation (JEP 213, Java 9).
   *
   * <p>JLS §15.9.5: Anonymous class declarations.
   * JEP: <a href="https://openjdk.org/jeps/213">JEP 213 – Milling Project Coin</a>
   */
  @Test
  void diamond_with_anonymous_class() throws Exception {
    TestStorage s = index(
        "import java.util.Comparator;\n"
        + "public class DiamondAnon {\n"
        + "  void m() {\n"
        + "    Comparator<String> c = new Comparator<>() {\n"
        + "      public int compare(String a, String b) { return a.compareTo(b); }\n"
        + "    };\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.classes.contains("public DiamondAnon <2:1 <2:14 2:24> 8:1>"),
        "outer class: " + s.classes);
    // The anonymous class is named by source position, so this is an exact string.
    assertTrue(s.methods.contains(
        "public int DiamondAnon.anonymous class (Test.java<4:28>)"
        + ".compare(java.lang.String, java.lang.String) <5:7 <5:18 5:24> 5:71>"),
        "anonymous class compare() method: " + s.methods);
    // the constructor call to the anonymous class is emitted as a call from m()
    assertTrue(s.calls.contains(
        "void DiamondAnon.m() -> void DiamondAnon.anonymous class (Test.java<4:28>)"
        + ".anonymous class (Test.java<4:28>)() <4:32 4:43>"),
        "call from m() to anonymous class constructor: " + s.calls);
  }

  // ---- JEP 213 / JLS 14.20.3 -- try-with-resources (effectively final) ---

  /**
   * Try-with-resources over an effectively-final variable (JEP 213, Java 9).
   *
   * <p>JLS §14.20.3: try-with-resources.
   * JEP: <a href="https://openjdk.org/jeps/213">JEP 213 – Milling Project Coin</a>
   */
  @Test
  void try_with_resources_effectively_final() throws Exception {
    TestStorage s = index(
        "public class TryRes {\n"
        + "  void m() throws Exception {\n"
        + "    AutoCloseable r = () -> {};\n"
        + "    try(r) {\n"
        + "    }\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.classes.contains("public TryRes <1:1 <1:14 1:19> 7:1>"),
        "TryRes class: " + s.classes);
    assertTrue(s.methods.contains("default void TryRes.m() <2:3 <2:8 2:8> 6:3>"),
        "m() method: " + s.methods);
    // r is an effectively-final local used in try(r); it appears as a local symbol
    assertTrue(s.localSymbols.contains("Test.java<3:19> <3:19 3:19>"),
        "r local symbol (declaration): " + s.localSymbols);
  }

  // ---- JLS 15.9.5 -- anonymous inner class with its own members -------

  /**
   * Anonymous inner class with its own field and method.
   *
   * <p>JLS §15.9.5: Anonymous class declarations; anonymous classes may declare members other
   * than constructors.
   * Reference: <a href="https://docs.oracle.com/javase/specs/jls/se11/html/jls-15.html#jls-15.9.5">JLS 11 §15.9.5</a>
   */
  @Test
  void anonymous_inner_class_members() throws Exception {
    TestStorage s = index(
        "public class Outer {\n"
        + "  Object make() {\n"
        + "    return new Object() {\n"
        + "      int x = 1;\n"
        + "      void helper() { int v = x; }\n"
        + "    };\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.classes.contains("public Outer <1:1 <1:14 1:18> 8:1>"),
        "outer class: " + s.classes);
    // the anonymous class field x is emitted as a member of Outer (flat hierarchy in the collector)
    assertTrue(s.fields.contains("default Outer.x <4:11 4:11>"),
        "anonymous class field x: " + s.fields);
    assertTrue(s.methods.contains(
        "default void Outer.anonymous class (Test.java<3:12>).helper() <5:7 <5:12 5:17> 5:34>"),
        "anonymous class helper() method: " + s.methods);
  }

  // ---- JLS 8.1.2 -- generic class with bounded type parameter ---------

  /**
   * Generic class with a bounded type parameter.
   *
   * <p>JLS §8.1.2: Generic classes and type parameters.
   * Reference: <a href="https://docs.oracle.com/javase/specs/jls/se11/html/jls-8.html#jls-8.1.2">JLS 11 §8.1.2</a>
   */
  @Test
  void generic_class_bounded_type_parameter() throws Exception {
    TestStorage s = index(
        "public class Box<T extends Comparable<T>> {\n"
        + "  private T value;\n"
        + "  public T get() { return value; }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    // ponytail: the typeParameters bin is always empty. visit(TypeParameter) calls declareLocal(),
    //           so the *declaration* of T becomes a local symbol with ACCESS_TYPE_PARAMETER rather
    //           than a NODE_TYPE_PARAMETER node. Fix: emit NODE_TYPE_PARAMETER there.
    assertTrue(s.typeParameters.isEmpty(),
        "known gap: T's declaration is a local symbol, not a type-parameter node: " + s.typeParameters);
    assertTrue(s.classes.contains("public Box <1:1 <1:14 1:16> 4:1>"),
        "Box class: " + s.classes);
    // ponytail: worse than the missing node -- every *use* of T (the field type, the return type)
    //           fabricates a bare class node named "T" in the current package, because
    //           typeReference() resolves the unqualified name lexically and cannot tell a type
    //           parameter from a real type. This is the same defect JavaIndexerTest already guards
    //           against for local variables (a_bare_local_variable_read_does_not_fabricate_a_class_node),
    //           still open for type parameters. Fix: skip names bound by an enclosing type parameter.
    assertTrue(s.classes.contains("T"),
        "uses of T fabricate a class node (collector gap): " + s.classes);
    assertTrue(s.fields.contains("private Box.value <2:13 2:17>"),
        "value field: " + s.fields);
    assertTrue(s.methods.contains("public T Box.get() <3:3 <3:12 3:14> 3:34>"),
        "get() method: " + s.methods);
  }
}
