package com.sourcetrail.indexer;

import static org.junit.jupiter.api.Assertions.assertEquals;

import org.junit.jupiter.api.Test;

/**
 * Byte-for-byte assertions against the engine's {@code NameHierarchy::serialize}
 * ({@code src/lib/lib/data/name/NameHierarchy.cpp}).
 *
 * <p>This is the first of the two contracts in {@code indexers/java/indexer/CLAUDE.md} that must not
 * drift, and it fails silently: the engine dedups nodes at merge time on
 * {@code (type, serializedName)}, so a formatting mismatch splits one symbol into two rather than
 * raising an error anywhere.
 *
 * <p>Layout, from the C++ source:
 * <pre>
 *   &lt;delimiter&gt; "\tm" name_1 "\ts" prefix_1 "\tp" postfix_1 ["\tn" name_2 "\ts" ...]
 * </pre>
 */
class NamesTest {

  @Test
  void single_plain_element_uses_the_java_delimiter() {
    assertEquals(".\tmFoo\ts\tp", Names.join(Names.Element.plain("Foo")));
  }

  @Test
  void elements_are_separated_by_the_name_delimiter() {
    assertEquals(".\tma\ts\tp\tnb\ts\tp\tnC\ts\tp",
        Names.join(Names.Element.plain("a"), Names.Element.plain("b"), Names.Element.plain("C")));
  }

  @Test
  void signature_prefix_and_postfix_land_in_their_own_slots() {
    assertEquals(".\tmm\tsvoid\tp(int)",
        Names.join(Names.Element.signature("m", "void", "(int)")));
  }

  @Test
  void signature_element_composes_with_a_qualifier_chain() {
    assertEquals(".\tma\ts\tp\tnFoo\ts\tp\tnm\tsvoid\tp(int, java.lang.String)",
        Names.join(concat(Names.split("a.Foo"),
            Names.Element.signature("m", "void", "(int, java.lang.String)"))));
  }

  @Test
  void file_names_use_the_file_delimiter() {
    assertEquals("/\tm/tmp/Foo.java\ts\tp", Names.file("/tmp/Foo.java"));
  }

  @Test
  void split_produces_one_element_per_dotted_part() {
    assertEquals(".\tmcom\ts\tp\tnexample\ts\tp\tnFoo\ts\tp", Names.join(Names.split("com.example.Foo")));
  }

  @Test
  void split_trims_leading_and_trailing_dots() {
    assertEquals(Names.join(Names.split("a.b")), Names.join(Names.split(".a.b.")));
  }

  @Test
  void split_of_the_empty_name_yields_the_default_placeholder() {
    assertEquals(".\tm<default>\ts\tp", Names.join(Names.split("")));
  }

  private static Names.Element[] concat(Names.Element[] base, Names.Element extra) {
    Names.Element[] out = new Names.Element[base.length + 1];
    System.arraycopy(base, 0, out, 0, base.length);
    out[base.length] = extra;
    return out;
  }
}
