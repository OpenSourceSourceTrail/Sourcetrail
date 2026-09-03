package com.sourcetrail.indexer;

import com.github.javaparser.ast.CompilationUnit;
import com.github.javaparser.ast.ImportDeclaration;
import com.github.javaparser.ast.type.ReferenceType;
import java.util.HashMap;
import java.util.Map;

/**
 * Resolves the fully qualified name of a reference using the compilation unit's
 * package declaration and import list (lexical resolution, no classpath required).
 *
 * <p>This gives cross-file-consistent names as long as every file that references a
 * given class uses the same fully-qualified spelling for it. Two kinds of names are
 * handled:
 *
 * <ul>
 *   <li>Qualified names as written ({@code java.util.List}) are used verbatim.</li>
 *   <li>Unqualified simple names are resolved via the import table; if no import
 *       matches, they are assumed to be in the current package.</li>
 * </ul>
 */
public final class NameResolver {
  private final String defaultPackage;
  private final Map<String, String> imports = new HashMap<>();

  public NameResolver(CompilationUnit cu) {
    if(cu != null && cu.getPackageDeclaration().isPresent()) {
      String pkg = cu.getPackageDeclaration().get().getNameAsString();
      this.defaultPackage = (pkg == null || pkg.isEmpty()) ? "" : pkg;
    } else {
      this.defaultPackage = "";
    }
    if(cu != null) {
      for(ImportDeclaration imp : cu.getImports()) {
        if(imp.isAsterisk()) {
          continue;
        }
        String name = imp.getNameAsString();
        if(name == null || name.isEmpty()) {
          continue;
        }
        int lastDot = name.lastIndexOf('.');
        if(lastDot >= 0) {
          String simple = name.substring(lastDot + 1);
          if(!imports.containsKey(simple)) {
            imports.put(simple, name);
          }
        }
      }
    }
  }

  /**
   * Resolve a simple or dotted name to its fully qualified form, assuming {@code name}
   * has at most one dotted segment (i.e. a single type or a package path).
   */
  public String resolveName(String name) {
    if(name == null || name.isEmpty()) {
      return "";
    }
    int firstDot = name.indexOf('.');
    String first = firstDot < 0 ? name : name.substring(0, firstDot);
    String rest = firstDot < 0 ? "" : name.substring(firstDot);

    String imported = imports.get(first);
    if(imported != null) {
      return imported + rest;
    }
    if(!defaultPackage.isEmpty()) {
      return defaultPackage + "." + name;
    }
    return name;
  }

  /** Resolve the FQN of a type reference (strips generics and array suffix). */
  public String resolveType(ReferenceType rt) {
    if(rt == null) {
      return "";
    }
    String s = rt.asString();
    int lt = s.indexOf('<');
    if(lt >= 0) {
      s = s.substring(0, lt);
    }
    int br = s.indexOf('[');
    if(br >= 0) {
      s = s.substring(0, br);
    }
    s = s.trim();
    return resolveName(s);
  }

  public String defaultPackage() {
    return defaultPackage;
  }

  /** True when the name can be resolved (via import or default package). */
  public boolean isResolvable(String name) {
    if(name == null || name.isEmpty()) {
      return false;
    }
    int firstDot = name.indexOf('.');
    String first = firstDot < 0 ? name : name.substring(0, firstDot);
    return imports.containsKey(first) || !defaultPackage.isEmpty();
  }
}
