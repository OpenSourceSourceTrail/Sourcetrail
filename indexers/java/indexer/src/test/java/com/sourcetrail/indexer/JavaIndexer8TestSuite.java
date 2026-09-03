package com.sourcetrail.indexer;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import org.junit.jupiter.api.Test;

/**
 * Java 8 language-feature tests for the JavaIndexer, structured after
 * {@code CxxParser11TestSuite.cpp} (one {@code @Test} per feature, Javadoc
 * linking the JLS/JSR/JEP, snippet in, bin assertion out).
 *
 * <p>Language level: JSR 335 (lambdas / method references / default interface
 * methods), JSR 308 (type annotations), JSR 310 (date-time API — no indexer
 * entries, so not tested), and JEP 120 / 176 / 104 (repeating annotations,
 * static interface methods, functional-interface annotation).
 *
 * <p>The negative test at the bottom proves that {@code IndexerCommand.language_standard = "8"}
 * is actually wired through {@code JavaIndexer.languageLevelOf}: a Java-21-only
 * record-pattern construct must fail to parse at standard "8".
 */
class JavaIndexer8TestSuite extends JavaStdTestSuite {
  @Override
  protected String standard() {
    return "8";
  }

  // ---- JSR 335: lambdas -----------------------------------------------

  /**
   * JLS §15.27 — Lambda expressions (JSR 335, Java SE 8).
   * A lambda body that calls {@code System.out.println} must produce a CALL
   * edge from the enclosing method. The lambda itself is anonymous; the
   * indexer does NOT emit a separate method node for the lambda body.
   *
   * @see <a href="https://jcp.org/en/jsr/detail?id=335">JSR 335</a>
   */
  @Test
  void lambdaExpressionEmitsCallEdge() throws IOException {
    TestStorage s = index(
        "public class Foo {\n"
        + "  void m() {\n"
        + "    Runnable r = () -> System.out.println(\"hi\");\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.calls.contains(
            "void Foo.m() -> void java.io.PrintStream.println(java.lang.String) <3:35 3:41>"),
        "lambda body call must produce a call edge from the enclosing method; got: " + s.calls);
  }

  // ---- JSR 335: method references -------------------------------------

  /**
   * JLS §15.13 — Static method reference (JSR 335, Java SE 8).
   * {@code String::valueOf} is sugar for a static method invocation, so it must reach the method
   * in the call graph exactly as {@code String.valueOf(x)} would. The location spans the whole
   * {@code scope::name} expression, since the identifier has no range of its own.
   *
   * @see <a href="https://jcp.org/en/jsr/detail?id=335">JSR 335</a>
   */
  @Test
  void staticMethodReferenceEmitsCallEdge() throws IOException {
    TestStorage s = index(
        "import java.util.function.Function;\n"
        + "public class Foo {\n"
        + "  void m() {\n"
        + "    Function<Integer, String> f = String::valueOf;\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.imports.contains("Test.java -> java.util.function.Function <1:1 1:35>"),
        "import edge must be emitted for java.util.function.Function; got: " + s.imports);
    assertTrue(
        s.calls.contains("void Foo.m() -> java.lang.String java.lang.String.valueOf(java.lang.Object) <4:35 4:49>"),
        "String::valueOf must reach the method it names; got: " + s.calls);
  }

  /**
   * JLS §15.13 — Instance method reference on {@code this} (JSR 335, Java SE 8).
   * The call edge must land on the declaration node in this same file, which is the case that
   * proves the reference and the declaration serialize to one and the same name.
   *
   * @see <a href="https://jcp.org/en/jsr/detail?id=335">JSR 335</a>
   */
  @Test
  void instanceMethodReferenceOnThisEmitsCallEdge() throws IOException {
    TestStorage s = index(
        "import java.util.function.Supplier;\n"
        + "public class Foo {\n"
        + "  String helper() { return \"\"; }\n"
        + "  void m() {\n"
        + "    Supplier<String> sup = this::helper;\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.methods.contains(
            "default java.lang.String Foo.helper() <3:3 <3:10 3:15> 3:32>"),
        "helper method must be emitted even when referenced via this::helper; got: " + s.methods);
    assertTrue(
        s.calls.contains("void Foo.m() -> java.lang.String Foo.helper() <5:28 5:39>"),
        "this::helper must reach the declaration in this file; got: " + s.calls);
  }

  /**
   * JLS §15.13 — Constructor reference (JSR 335, Java SE 8).
   *
   * <p>ponytail: JavaParser 3.26.4 cannot resolve a constructor reference ("Constructor calls not
   * yet resolvable"), so {@code JavaCollector} records the type being constructed and emits no
   * call edge, rather than fabricating a {@code new()} method node matching no declaration. Flip
   * this to a call-edge assertion if a JavaParser upgrade makes it resolvable.
   *
   * @see <a href="https://jcp.org/en/jsr/detail?id=335">JSR 335</a>
   */
  @Test
  void constructorReferenceEmitsTypeUseNotACall() throws IOException {
    TestStorage s = index(
        "import java.util.function.Supplier;\n"
        + "public class Foo {\n"
        + "  void m() {\n"
        + "    Supplier<String> sup = String::new;\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.typeUses.contains("void Foo.m() -> java.lang.String <4:28 4:33>"),
        "the constructed type must still be recorded; got: " + s.typeUses);
    assertTrue(
        s.calls.isEmpty(),
        "an unresolvable constructor reference must not fabricate a call; got: " + s.calls);
  }

  // ---- JSR 335: default interface method ------------------------------

  /**
   * JLS §9.4.3 — Default methods in interfaces (JSR 335, Java SE 8).
   * A {@code default} method is a concrete method on an interface; the
   * indexer emits a METHOD node exactly as for a class method.
   *
   * @see <a href="https://jcp.org/en/jsr/detail?id=335">JSR 335</a>
   */
  @Test
  void defaultInterfaceMethodEmitsMethodNode() throws IOException {
    TestStorage s = index(
        "public interface Greeter {\n"
        + "  default String greet(String name) {\n"
        + "    return \"Hello \" + name;\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.interfaces.contains("public Greeter <1:1 <1:18 1:24> 5:1>"),
        "interface node must be emitted; got: " + s.interfaces);
    assertTrue(
        s.methods.contains(
            "public java.lang.String Greeter.greet(java.lang.String) <2:3 <2:18 2:22> 4:3>"),
        "default interface method must emit a method node; got: " + s.methods);
  }

  // ---- JSR 335: static interface method -------------------------------

  /**
   * JLS §9.4 — Static methods in interfaces (JEP 176 / JSR 335, Java SE 8).
   * A {@code static} method declared in an interface is emitted as a METHOD
   * node owned by the interface node via a MEMBER edge.
   *
   * @see <a href="https://openjdk.org/jeps/176">JEP 176</a>
   */
  @Test
  void staticInterfaceMethodEmitsMethodNode() throws IOException {
    TestStorage s = index(
        "public interface MathOp {\n"
        + "  static int add(int a, int b) { return a + b; }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.methods.contains(
            "public int MathOp.add(int, int) <2:3 <2:14 2:16> 2:48>"),
        "static interface method must emit a method node; got: " + s.methods);
  }

  // ---- JEP 104: @FunctionalInterface ----------------------------------

  /**
   * JLS §9.8 + JEP 104 — {@code @FunctionalInterface} annotation (Java SE 8).
   * An interface carrying this annotation is recorded as an INTERFACE node;
   * the annotation produces an ANNOTATION_USAGE edge from the interface node
   * to the {@code java.lang.FunctionalInterface} annotation node.
   *
   * @see <a href="https://openjdk.org/jeps/104">JEP 104</a>
   */
  @Test
  void functionalInterfaceAnnotationEmitsAnnotationUseEdge() throws IOException {
    TestStorage s = index(
        "@FunctionalInterface\n"
        + "public interface Transformer {\n"
        + "  String transform(String s);\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.interfaces.contains("public Transformer <1:1 <2:18 2:28> 4:1>"),
        "FunctionalInterface interface must emit an interface node; got: " + s.interfaces);
    assertTrue(
        s.annotationUses.contains("Transformer -> java.lang.FunctionalInterface <1:1 1:20>"),
        "@FunctionalInterface must emit an annotation-usage edge; got: " + s.annotationUses);
  }

  // ---- JEP 120: repeating annotations ---------------------------------

  /**
   * JLS §9.6.3 + JEP 120 — Repeating annotations (Java SE 8).
   * Two occurrences of the same annotation on one declaration produce two
   * ANNOTATION_USAGE edges from that declaration node.
   *
   * @see <a href="https://openjdk.org/jeps/120">JEP 120</a>
   */
  @Test
  void repeatingAnnotationsEmitTwoAnnotationUseEdges() throws IOException {
    TestStorage s = index(
        "import java.lang.annotation.*;\n"
        + "@Repeatable(Hints.class)\n"
        + "@interface Hint { String value(); }\n"
        + "@interface Hints { Hint[] value(); }\n"
        + "@Hint(\"a\") @Hint(\"b\")\n"
        + "public class Foo { }\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.annotationUses.contains("Foo -> Hint <5:1 5:10>"),
        "first @Hint usage must be recorded; got: " + s.annotationUses);
    assertTrue(
        s.annotationUses.contains("Foo -> Hint <5:12 5:21>"),
        "second @Hint usage must be recorded; got: " + s.annotationUses);
  }

  // ---- JSR 308: type annotations --------------------------------------

  /**
   * JLS §9.7.4 + JSR 308 — Type annotations (Java SE 8).
   * An annotation in a type-use position (e.g. {@code @NonNull String}) is a
   * type annotation; the indexer must parse it without error, emit the field
   * node, and record the annotation-usage edge.
   *
   * @see <a href="https://jcp.org/en/jsr/detail?id=308">JSR 308</a>
   */
  @Test
  void typeAnnotationParsesAndEmitsFieldAndAnnotationUse() throws IOException {
    TestStorage s = index(
        "import java.lang.annotation.*;\n"
        + "@Target({ElementType.TYPE_USE}) @interface NonNull {}\n"
        + "public class Foo {\n"
        + "  @NonNull String name = \"\";\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.fields.contains("default Foo.name <4:19 4:22>"),
        "field with type annotation must still emit a field node; got: " + s.fields);
    assertTrue(
        s.annotationUses.contains("Foo.name -> NonNull <4:3 4:10>"),
        "type annotation must emit an annotation-usage edge; got: " + s.annotationUses);
  }

  // ---- Generic method with inferred type arguments --------------------

  /**
   * JLS §8.4.4 — Generic methods; type-argument inference improved in Java 8
   * (JEP 101). A generic method call with inferred type arguments must produce
   * a CALL edge from the enclosing method to the generic method.
   *
   * <p>ponytail: {@code typeArguments} bin is empty — JavaCollector does not
   * emit TYPE_ARGUMENT edges for generic method calls. Add when
   * JavaCollector tracks type arguments on call sites.
   *
   * @see <a href="https://openjdk.org/jeps/101">JEP 101</a>
   */
  @Test
  void genericMethodCallWithInferredTypeArgumentsEmitsCallEdge() throws IOException {
    TestStorage s = index(
        "import java.util.Arrays;\n"
        + "import java.util.List;\n"
        + "public class Foo {\n"
        + "  void m() {\n"
        + "    List<String> list = Arrays.asList(\"a\", \"b\");\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.calls.contains(
            "void Foo.m() -> java.util.List<T> java.util.Arrays.asList(T[]) <5:32 5:37>"),
        "generic method call must produce a call edge; got: " + s.calls);
    // ponytail: TYPE_ARGUMENT edges not emitted for generic call sites — JavaCollector skips them
  }

  // ---- enhanced-for with effectively-final capture --------------------

  /**
   * JLS §14.14.2 + JLS §6.5.6.1 — Enhanced-for over an array; effectively
   * final (Java SE 8, JLS formalized "effectively final" in JSR 335).
   * A lambda inside an enhanced-for that captures the loop variable (which is
   * effectively final) must parse without error.
   *
   * <p>ponytail: call edge for the lambda body inside enhanced-for is not
   * emitted — JavaCollector does not recurse into lambda bodies that appear
   * inside for-each statements. Add when lambda-body traversal is fixed.
   *
   * @see <a href="https://docs.oracle.com/javase/specs/jls/se8/html/jls-14.html#jls-14.14.2">JLS §14.14.2</a>
   */
  @Test
  void enhancedForWithEffectivelyFinalCaptureParsesWithoutError() throws IOException {
    TestStorage s = index(
        "import java.util.function.Supplier;\n"
        + "public class Foo {\n"
        + "  void m() {\n"
        + "    String[] items = {\"a\", \"b\"};\n"
        + "    for (String item : items) {\n"
        + "      Supplier<String> sup = () -> item;\n"
        + "    }\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(
        s.methods.contains("default void Foo.m() <3:3 <3:8 3:8> 8:3>"),
        "enclosing method must be emitted; got: " + s.methods);
    // ponytail: call edge for lambda body inside enhanced-for not emitted — JavaCollector
    //           does not recurse into lambda bodies in for-each; fix traversal to emit EDGE_CALL
  }

  // ---- negative test: Java-21 record pattern fails at standard "8" ----

  /**
   * JEP 440 — Record patterns (Java SE 21, preview in 19/20).
   * When the indexer is configured with {@code language_standard = "8"} the
   * parser must reject Java-21-only syntax (a deconstruction pattern in an
   * {@code instanceof} expression). This proves that
   * {@code IndexerCommand.language_standard} is wired through
   * {@code JavaIndexer.languageLevelOf}: without that wiring all per-LTS
   * test suites are unverified.
   *
   * @see <a href="https://openjdk.org/jeps/440">JEP 440</a>
   */
  @Test
  void java21RecordPatternFailsAtStandard8() throws IOException {
    // record Point(...){} is fine in Java 16+; the deconstruction pattern
    // `o instanceof Point(int x, int y)` requires Java 21.
    TestStorage s = index(
        "public class Foo {\n"
        + "  record Point(int x, int y) {}\n"
        + "  void m(Object o) {\n"
        + "    if (o instanceof Point(int x, int y)) { int sum = x + y; }\n"
        + "  }\n"
        + "}\n");
    assertTrue(s.proto().getErrorsCount() > 0,
        "Java-21 record-pattern syntax must fail to parse at language standard 8");
    assertFalse(s.proto().getFiles(0).getComplete(),
        "a file that fails to parse must not be marked complete");
  }
}
