package com.sourcetrail.indexer;

/**
 * Builds byte-compatible strings with the engine's
 * {@code NameHierarchy::serialize}/{@code deserialize}
 * ({@code src/lib/lib/data/name/NameHierarchy.cpp}). That serialized form is what
 * the C++ read path re-hydrates on every name lookup and the key used to
 * deduplicate nodes at merge time (storage::addNode keys on (type, name)).
 *
 * <p>Serialized layout (tab-delimited):
 * <pre>
 *   &lt;delimiter&gt; \tm
 *   [name_1 \ts sigprefix_1 \tp sigpostfix_1]
 *   [\tn name_2 \ts sigprefix_2 \tp sigpostfix_2]
 *   ...
 * </pre>
 *
 * <p>The Java delimiter is {@code "."} ({@code NAME_DELIMITER_JAVA == "."});
 * the file delimiter is {@code "/"} ({@code NAME_DELIMITER_FILE == "/"}).
 */
public final class Names {
  private Names() {}

  private static final char TAB = '\t';
  private static final String META = TAB + "m";
  private static final String NAME = TAB + "n";
  private static final String PART = TAB + "s";
  private static final String SIGN = TAB + "p";
  private static final char JAVA_DELIM = '.';
  private static final char FILE_DELIM = '/';

  /** One element of a name hierarchy: name + optional signature prefix/postfix. */
  public static final class Element {
    public final String name;
    public final String prefix;
    public final String postfix;

    private Element(String name, String prefix, String postfix) {
      this.name = name == null ? "" : name;
      this.prefix = prefix == null ? "" : prefix;
      this.postfix = postfix == null ? "" : postfix;
    }

    public static Element plain(String name) {
      return new Element(name, "", "");
    }

    public static Element signature(String name, String prefix, String postfix) {
      return new Element(name, prefix, postfix);
    }
  }

  /** Serialize one or more elements with the Java {@code .} delimiter. */
  public static String join(Element... elements) {
    return join(JAVA_DELIM, elements);
  }

  /** Split a dotted fully-qualified name into name elements (one per part). */
  public static Element[] split(String dotted) {
    if(dotted == null) {
      dotted = "";
    }
    if(dotted.startsWith(".")) {
      dotted = dotted.substring(1);
    }
    if(dotted.endsWith(".")) {
      dotted = dotted.substring(0, dotted.length() - 1);
    }
    if(dotted.isEmpty()) {
      return new Element[]{Element.plain("<default>")};
    }
    String[] parts = dotted.split("\\.", -1);
    Element[] elements = new Element[parts.length];
    for(int i = 0; i < parts.length; i++) {
      elements[i] = Element.plain(parts[i]);
    }
    return elements;
  }

  /** Append one element to a chain. */
  public static Element[] concat(Element[] base, Element extra) {
    Element[] out = new Element[base.length + 1];
    System.arraycopy(base, 0, out, 0, base.length);
    out[base.length] = extra;
    return out;
  }

  /** Serialize a single name with the file {@code /} delimiter. */
  public static String file(String path) {
    return join(FILE_DELIM, new Element[]{Element.plain(path == null ? "" : path)});
  }

  private static String join(char delimiter, Element[] elements) {
    StringBuilder sb = new StringBuilder();
    sb.append(delimiter).append(META);
    for(int i = 0; i < elements.length; i++) {
      if(i > 0) {
        sb.append(NAME);
      }
      Element e = elements[i];
      sb.append(e.name).append(PART).append(e.prefix).append(SIGN).append(e.postfix);
    }
    return sb.toString();
  }
}
