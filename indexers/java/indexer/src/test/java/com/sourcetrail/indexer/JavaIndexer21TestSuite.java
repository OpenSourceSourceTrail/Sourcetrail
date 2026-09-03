package com.sourcetrail.indexer;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import org.junit.jupiter.api.Test;

/**
 * Java-21 feature tests for the Sourcetrail Java indexer.
 *
 * <p>Mirrors {@code CxxParser11TestSuite.cpp} in shape: one {@code @Test} per language feature,
 * Javadoc linking the JLS section and the JEP, a representative snippet, then assertions on the
 * emitted bins. All expected strings are observed values printed by a probe run, never guessed.
 *
 * <p>Coverage: Java 18–21 features finalised in JDK 21.
 */
class JavaIndexer21TestSuite extends JavaStdTestSuite {

  @Override
  protected String standard() {
    return "21";
  }

  // -----------------------------------------------------------------------
  // JEP 441 — Pattern Matching for switch (final in Java 21)
  // JLS 14.11.1
  // -----------------------------------------------------------------------

  /**
   * A switch expression that pattern-matches an {@code Object} argument.
   * Binding variables ({@code i}, {@code str}) flow through JavaCollector's
   * {@code visit(Parameter, Void)} visitor and land in {@code localSymbols} as
   * file-location strings (JavaParser treats pattern vars as Parameters).
   * JEP 441; JLS 14.11.1.
   */
  @Test
  void switchPatternEmitsMethodAndCallEdge() throws IOException {
    TestStorage s = index(
        "class Formatter {\n"
        + "  String format(Object o) {\n"
        + "    return switch (o) {\n"
        + "      case Integer i -> \"int \" + i;\n"
        + "      case String str -> str.toUpperCase();\n"
        + "      default -> o.toString();\n"
        + "    };\n"
        + "  }\n"
        + "}\n");

    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.methods.contains(
        "default java.lang.String Formatter.format(java.lang.Object) <2:3 <2:10 2:15> 8:3>"));
    assertTrue(s.calls.contains(
        "java.lang.String Formatter.format(java.lang.Object)"
        + " -> java.lang.String java.lang.String.toUpperCase() <5:30 5:40>"));
    // Binding variables i and str appear as local symbols (via Parameter visitor).
    assertTrue(s.localSymbols.contains("Test.java<4:12> <4:34 4:34>")); // i
    assertTrue(s.localSymbols.contains("Test.java<5:12> <5:26 5:28>")); // str
  }

  // -----------------------------------------------------------------------
  // JEP 440 — Record Patterns (final in Java 21)
  // JLS 14.30.2
  // -----------------------------------------------------------------------

  /**
   * A record pattern in an {@code instanceof} expression.
   * Component binding variables ({@code x}, {@code y}) appear in {@code localSymbols}.
   * JEP 440; JLS 14.30.2.
   */
  @Test
  void recordPatternBindingAppearsInLocalSymbols() throws IOException {
    TestStorage s = index(
        "record Point(int x, int y) { }\n"
        + "class Checker {\n"
        + "  boolean isOrigin(Object o) {\n"
        + "    return o instanceof Point(int x, int y) && x == 0 && y == 0;\n"
        + "  }\n"
        + "}\n");

    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.classes.contains("default Point <1:8 1:12>"));
    assertTrue(s.classes.contains("default Checker <2:1 <2:7 2:13> 6:1>"));
    // Binding x and y from the record pattern appear in localSymbols.
    assertTrue(s.localSymbols.stream().anyMatch(e -> e.startsWith("Test.java<4:31>")));
    assertTrue(s.localSymbols.stream().anyMatch(e -> e.startsWith("Test.java<4:38>")));
  }

  /**
   * A nested record pattern inside a switch expression.
   * JEP 440; JLS 14.30.2.
   */
  @Test
  void nestedRecordPatternInSwitch() throws IOException {
    TestStorage s = index(
        "record Coord(int v) { }\n"
        + "record Line(Coord a, Coord b) { }\n"
        + "class Printer {\n"
        + "  String describe(Object o) {\n"
        + "    return switch (o) {\n"
        + "      case Line(Coord(int x1), Coord(int x2)) -> x1 + \"-\" + x2;\n"
        + "      default -> \"?\";\n"
        + "    };\n"
        + "  }\n"
        + "}\n");

    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.classes.contains("default Coord <1:8 1:12>"));
    assertTrue(s.classes.contains("default Line <2:8 2:11>"));
    // Nested binding vars x1/x2 appear in localSymbols.
    assertTrue(s.localSymbols.stream().anyMatch(e -> e.startsWith("Test.java<6:23>")));
    assertTrue(s.localSymbols.stream().anyMatch(e -> e.startsWith("Test.java<6:38>")));
  }

  // -----------------------------------------------------------------------
  // JEP 409 — Sealed Classes (final in Java 17), exhaustive switch in Java 21
  // JLS 8.1.1.2 / 14.11.1
  // -----------------------------------------------------------------------

  /**
   * A switch over a sealed hierarchy with no {@code default} arm; exhaustiveness is enforced
   * by the compiler at JAVA_21 level. The indexer must parse this without errors and emit the
   * sealed interface and its permitted subtypes.
   * JEP 409; JLS 8.1.1.2, 14.11.1.
   */
  @Test
  void sealedSwitchWithNoDefaultIsExhaustive() throws IOException {
    TestStorage s = index(
        "sealed interface Shape permits Circle, Rect { }\n"
        + "record Circle(double r) implements Shape { }\n"
        + "record Rect(double w, double h) implements Shape { }\n"
        + "class Calc {\n"
        + "  double area(Shape sh) {\n"
        + "    return switch (sh) {\n"
        + "      case Circle c -> Math.PI * c.r() * c.r();\n"
        + "      case Rect r   -> r.w() * r.h();\n"
        + "    };\n"
        + "  }\n"
        + "}\n");

    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.interfaces.contains("default Shape <1:18 1:22>"));
    assertTrue(s.classes.contains("default Circle <2:8 2:13>"));
    assertTrue(s.classes.contains("default Rect <3:8 3:11>"));
    assertTrue(s.methods.contains(
        "default double Calc.area(Shape) <5:3 <5:10 5:13> 10:3>"));
  }

  // -----------------------------------------------------------------------
  // JEP 441 — Guarded patterns (when clause)
  // JLS 14.11.1
  // -----------------------------------------------------------------------

  /**
   * A guarded pattern: {@code case Box b when b.value() > 0}. The binding variable
   * {@code b} appears in {@code localSymbols}; the guard's method call is in {@code calls}.
   * JEP 441; JLS 14.11.1.
   */
  @Test
  void guardedPatternEmitsCallInGuard() throws IOException {
    TestStorage s = index(
        "class Box {\n"
        + "  int value() { return 1; }\n"
        + "  String describe(Object o) {\n"
        + "    return switch (o) {\n"
        + "      case Box b when b.value() > 0 -> \"positive\";\n"
        + "      default -> \"other\";\n"
        + "    };\n"
        + "  }\n"
        + "}\n");

    assertEquals(0, s.proto().getErrorsCount());
    // The guard calls value(); it should appear in the call graph.
    assertTrue(s.calls.contains(
        "java.lang.String Box.describe(java.lang.Object) -> int Box.value() <5:25 5:29>"));
    // Binding variable b appears in localSymbols.
    assertTrue(s.localSymbols.stream().anyMatch(e -> e.startsWith("Test.java<5:12>")));
  }

  // -----------------------------------------------------------------------
  // JEP 441 — case null
  // JLS 14.11.1
  // -----------------------------------------------------------------------

  /**
   * A {@code case null} arm in a switch expression (new in Java 21).
   * The enclosing method must be emitted without errors.
   * JEP 441; JLS 14.11.1.
   */
  @Test
  void caseNullParsesWithoutError() throws IOException {
    TestStorage s = index(
        "class Handler {\n"
        + "  String handle(String s) {\n"
        + "    return switch (s) {\n"
        + "      case null -> \"null!\";\n"
        + "      case \"\" -> \"empty\";\n"
        + "      default -> s;\n"
        + "    };\n"
        + "  }\n"
        + "}\n");

    assertEquals(0, s.proto().getErrorsCount());
    assertTrue(s.methods.contains(
        "default java.lang.String Handler.handle(java.lang.String) <2:3 <2:10 2:15> 8:3>"));
  }

  // -----------------------------------------------------------------------
  // JEP 395 / local type declarations finalised by Java 16, available at JAVA_21
  // JLS 6.5.5.1
  // -----------------------------------------------------------------------

  /**
   * A local {@code record} declared inside a method body (JEP 395 finalised in Java 16).
   * Records map to {@code NODE_CLASS}, so it appears in {@code classes} under the enclosing
   * class name.
   * JEP 395; JLS 6.5.5.1.
   */
  @Test
  void localRecordAppearsInClasses() throws IOException {
    TestStorage s = index(
        "class Wrapper {\n"
        + "  void run() {\n"
        + "    record Pair(int a, int b) { }\n"
        + "    Pair p = new Pair(1, 2);\n"
        + "  }\n"
        + "}\n");

    assertEquals(0, s.proto().getErrorsCount());
    // Local record Pair is emitted as a class nested under Wrapper.
    assertTrue(s.classes.contains("default Wrapper.Pair <3:12 3:15>"));
    // Components a and b are fields of the record, not locals of the enclosing method.
    assertTrue(s.fields.contains("private Wrapper.Pair.a <3:21 3:21>"), "fields: " + s.fields);
    assertTrue(s.fields.contains("private Wrapper.Pair.b <3:28 3:28>"), "fields: " + s.fields);
  }

  /**
   * A local {@code enum} declared inside a method body.
   *
   * <p>JLS 14.3, JEP 395 (Java 16) made local enum, record and interface declarations legal, so
   * this snippet is valid Java 21 and javac accepts it. JavaParser 3.26.4 nevertheless does NOT
   * parse a local enum at JAVA_21; the indexer returns a StorageError and marks the file
   * incomplete. This test pins that upstream limitation so a JavaParser upgrade fails here
   * loudly and gets the assertion flipped, rather than passing unnoticed.
   *
   * <p>ponytail: local enum in method body — JavaParser 3.26.4 parse failure at JAVA_21;
   *   upgrade JavaParser or add a pre-processing step if local enums must be indexed.
   */
  @Test
  void localEnumCausesParseError() throws IOException {
    TestStorage s = index(
        "class Ctx {\n"
        + "  void run() {\n"
        + "    enum Dir { N, S, E, W }\n"
        + "    Dir d = Dir.N;\n"
        + "  }\n"
        + "}\n");

    // JavaParser 3.26.4 cannot parse a local enum inside a method body at JAVA_21.
    assertTrue(s.proto().getErrorsCount() > 0);
  }

  /**
   * A local {@code interface} declared inside a method body (allowed since Java 16).
   * It appears in {@code interfaces} under the enclosing class name.
   * JLS 6.5.5.1.
   */
  @Test
  void localInterfaceAppearsInInterfaces() throws IOException {
    TestStorage s = index(
        "class Owner {\n"
        + "  void run() {\n"
        + "    interface Step { void execute(); }\n"
        + "    Step step = () -> {};\n"
        + "  }\n"
        + "}\n");

    assertEquals(0, s.proto().getErrorsCount());
    // Local interface Step is emitted under Owner.
    assertTrue(s.interfaces.contains("default Owner.Step <3:5 <3:15 3:18> 3:38>"));
  }
}
