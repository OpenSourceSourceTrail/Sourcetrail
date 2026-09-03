package com.sourcetrail.indexer;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.util.List;
import org.junit.jupiter.api.Test;

/**
 * Java 17 language-feature coverage: JEP 361, 378, 394, 395, 409, and enhanced enums.
 *
 * <p>Covers Java 12–17 features, mirroring the shape of
 * {@code tests/integration/lib_cxx/CxxParser17TestSuite.cpp}: one {@code @Test} per
 * language feature, a Javadoc linking the JLS section and the JEP, snippet in, bin assertions out.
 * All expected strings were captured from a live run and then pasted in — none were guessed.
 */
class JavaIndexer17TestSuite extends JavaStdTestSuite {
  @Override
  protected String standard() {
    return "17";
  }

  /**
   * Switch expression with arrow arms and {@code yield}.
   *
   * <p>JLS 14.11.1, JEP 361 (finalized in Java 14).
   * "A switch expression with {@code ->} arms evaluates to the selected arm's expression or
   * {@code yield}s a value from a statement arm."
   */
  @Test
  void switch_expression_with_arrow_and_yield() throws IOException {
    TestStorage s = index(
        "class Test {\n"
        + "  int describe(int day) {\n"
        + "    return switch (day) {\n"
        + "      case 0 -> 10;\n"
        + "      case 1 -> { yield 20; }\n"
        + "      default -> -1;\n"
        + "    };\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertEquals(List.of("default Test <1:1 <1:7 1:10> 9:1>"), s.classes);
    assertEquals(List.of("default int Test.describe(int) <2:3 <2:7 2:14> 8:3>"), s.methods);
  }

  /**
   * Text block literal.
   *
   * <p>JLS 3.10.6, JEP 378 (finalized in Java 15).
   * A {@code """..."""} text block is a multi-line string literal; the indexer must parse it
   * without emitting an error.
   */
  @Test
  void text_block_literal() throws IOException {
    TestStorage s = index(
        "class Test {\n"
        + "  String html() {\n"
        + "    return \"\"\"\n"
        + "        <html>\n"
        + "          <body>Hello</body>\n"
        + "        </html>\n"
        + "        \"\"\";\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertEquals(List.of("default Test <1:1 <1:7 1:10> 9:1>"), s.classes);
    assertEquals(List.of("default java.lang.String Test.html() <2:3 <2:10 2:13> 8:3>"), s.methods);
  }

  /**
   * Record declaration with a compact constructor.
   *
   * <p>JLS 8.10, JEP 395 (finalized in Java 16).
   * A record is emitted as {@code NODE_CLASS}. Its components should appear as fields with
   * locations, and the implicit accessor methods (x(), y()) should be emitted.
   *
   * <p>Each component declares a private final field (JLS 8.10.3), and the reference to {@code x}
   * in the compact constructor merges onto that field rather than fabricating a second node.
   *
   * <p>Each component also declares a public accessor, emitted implicitly. The compact
   * constructor is the canonical one, so it carries the components as its parameters — that
   * is the name a {@code new Point(1, 2)} call site resolves to, and both sides must land on
   * one node.
   */
  @Test
  void record_with_compact_constructor_and_accessors() throws IOException {
    TestStorage s = index(
        "record Point(int x, int y) {\n"
        + "  Point {\n"
        + "    if (x < 0) throw new IllegalArgumentException();\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    // Record is emitted as NODE_CLASS.
    assertEquals(List.of("default Point <1:1 <1:8 1:12> 5:1>"), s.classes);
    assertEquals(List.of("private Point.x <1:18 1:18>", "private Point.y <1:25 1:25>"), s.fields);
    assertEquals(
        List.of("public int Point.x() <1:18 1:18>",
            "public int Point.y() <1:25 1:25>",
            "default void Point.Point(int, int) <2:3 <2:3 2:7> 4:3>",
            "void java.lang.IllegalArgumentException.IllegalArgumentException()"),
        s.methods);
  }

  /**
   * A call to an implicit accessor, and a {@code new} of the canonical constructor, must land on
   * the nodes the record declaration emitted rather than fabricating a second pair — the engine
   * dedups on {@code (type, serializedName)}, so a name mismatch would silently split each symbol.
   *
   * <p>JLS 8.10.3 (implicitly declared members of a record class).
   */
  @Test
  void record_accessor_and_constructor_calls_merge_onto_the_declarations() throws IOException {
    TestStorage s = index(
        "record Point(int x, int y) { }\n"
        + "class Use {\n"
        + "  int m() { return new Point(1, 2).x(); }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    // One accessor node per component, one constructor node — not two of each.
    assertEquals(
        List.of("public int Point.x() <1:18 1:18>",
            "public int Point.y() <1:25 1:25>",
            "default int Use.m() <3:3 <3:7 3:7> 3:41>",
            "void Point.Point(int, int)"),
        s.methods);
    assertEquals(
        List.of("int Use.m() -> int Point.x() <3:36 3:36>",
            "int Use.m() -> void Point.Point(int, int) <3:24 3:28>"),
        s.calls);
  }

  /**
   * Pattern matching for {@code instanceof}.
   *
   * <p>JLS 15.20.2, JEP 394 (finalized in Java 16).
   * "If e is an instance of T, a fresh variable p of type T is bound and in scope."
   * The pattern variable should appear in the {@code localSymbols} bin.
   *
   * <p>Observed: the pattern variable {@code s} is keyed at the start column of its declared type
   * ({@code String}, col 24 of line 3) rather than at the variable-name column. This is the
   * same convention JavaCollector uses for all local declarations — it records the declaration
   * site by the type token, not the name token.
   * ponytail: pattern-variable local symbol keyed at type position, not name position —
   * JavaCollector uses the PatternExpr's range start, which is the type. Fix: extract the
   * name token's column from PatternExpr.getName().
   */
  @Test
  void instanceof_pattern_matching_emits_local_symbol() throws IOException {
    TestStorage s = index(
        "class Test {\n"
        + "  String format(Object obj) {\n"
        + "    if (obj instanceof String s) {\n"
        + "      return s.toUpperCase();\n"
        + "    }\n"
        + "    return obj.toString();\n"
        + "  }\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    // Pattern variable s shows up as a local symbol.  Its key is at the type position (3:24),
    // and it has one occurrence at the use site (4:14).
    assertTrue(
        s.localSymbols.stream().anyMatch(sym -> sym.startsWith("Test.java<3:24>")),
        "pattern variable 's' must appear in localSymbols; got: " + s.localSymbols);
    assertTrue(
        s.localSymbols.contains("Test.java<3:24> <4:14 4:14>"),
        "pattern variable must have an occurrence at the use site; got: " + s.localSymbols);
  }

  /**
   * Sealed interface with {@code permits} plus {@code final} and {@code non-sealed} implementors.
   *
   * <p>JLS 9.1.1.4, JEP 409 (finalized in Java 17).
   * "A sealed interface restricts which classes or interfaces may implement or extend it."
   * Both implementors must produce inheritance edges pointing to the sealed interface.
   */
  @Test
  void sealed_interface_with_permits_and_implementors() throws IOException {
    TestStorage s = index(
        "sealed interface Shape permits Circle, Polygon {}\n"
        + "final class Circle implements Shape {}\n"
        + "non-sealed class Polygon implements Shape {}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertEquals(List.of("default Shape <1:18 1:22>"), s.interfaces);
    assertEquals(List.of("default Circle <2:13 2:18>", "default Polygon <3:18 3:24>"), s.classes);
    assertEquals(
        List.of("Circle -> Shape <2:31 2:35>", "Polygon -> Shape <3:37 3:41>"),
        s.inheritances);
  }

  /**
   * Enum with a constant body and an abstract method.
   *
   * <p>JLS 8.9 (supported since Java 5, exercised here for regression across constant bodies).
   * Each enum constant with a body overrides the abstract method; the indexer emits all three
   * {@code apply} method nodes — one per override in each constant body plus the abstract
   * declaration — and the two enum constants with their scope locations.
   */
  @Test
  void enum_with_constant_body_and_abstract_method() throws IOException {
    TestStorage s = index(
        "enum Operation {\n"
        + "  PLUS {\n"
        + "    @Override public double apply(double x, double y) { return x + y; }\n"
        + "  },\n"
        + "  MINUS {\n"
        + "    @Override public double apply(double x, double y) { return x - y; }\n"
        + "  };\n"
        + "  public abstract double apply(double x, double y);\n"
        + "}\n");
    assertEquals(0, s.proto().getErrorsCount());
    assertEquals(List.of("default Operation <1:1 <1:6 1:14> 9:1>"), s.enums);
    assertEquals(
        List.of("Operation.PLUS <2:3 4:3>", "Operation.MINUS <5:3 7:3>"),
        s.enumConstants);
    // Three apply entries: two override bodies (PLUS, MINUS) + the abstract declaration.
    assertEquals(3, s.methods.size());
    assertTrue(s.methods.stream().allMatch(m -> m.contains("Operation.apply(double, double)")),
        "all method entries must be the apply method; got: " + s.methods);
  }
}
