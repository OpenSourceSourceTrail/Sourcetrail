package com.sourcetrail.indexer;

import com.github.javaparser.Range;
import com.github.javaparser.ast.Node;
import com.github.javaparser.ast.NodeList;
import com.github.javaparser.ast.body.AnnotationDeclaration;
import com.github.javaparser.ast.body.CallableDeclaration;
import com.github.javaparser.ast.body.ClassOrInterfaceDeclaration;
import com.github.javaparser.ast.body.ConstructorDeclaration;
import com.github.javaparser.ast.body.EnumDeclaration;
import com.github.javaparser.ast.body.MethodDeclaration;
import com.github.javaparser.ast.body.Parameter;
import com.github.javaparser.ast.body.TypeDeclaration;
import com.github.javaparser.ast.expr.ObjectCreationExpr;
import com.github.javaparser.ast.type.Type;
import com.github.javaparser.ast.type.TypeParameter;
import com.github.javaparser.resolution.TypeSolver;
import com.github.javaparser.resolution.declarations.ResolvedAnnotationDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedConstructorDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedMethodDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedMethodLikeDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedReferenceTypeDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedTypeParameterDeclaration;
import com.github.javaparser.resolution.model.SymbolReference;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

/**
 * Serialized names and node kinds for the symbols {@link JavaCollector} emits.
 *
 * <p>Node identity for the engine merge step is ({@code type, serializedName}), so a declaration in
 * one file and a reference to it in another must produce the identical string. Both sides go
 * through this class: names come from the <em>resolved</em> declaration wherever the symbol solver
 * can supply one, and fall back to the lexical {@link NameResolver} form otherwise.
 *
 * <p>Nothing here touches {@link Storage}; it computes strings and kinds only.
 */
final class SymbolNames {
  private final NameResolver resolver;
  private final TypeSolver typeSolver;
  private final String fileName;

  SymbolNames(NameResolver resolver, TypeSolver typeSolver, String filePath) {
    this.resolver = resolver;
    this.typeSolver = typeSolver;
    this.fileName = fileNameOf(filePath);
  }

  private static String fileNameOf(String filePath) {
    if(filePath == null || filePath.isEmpty()) {
      return "";
    }
    try {
      Path name = Path.of(filePath).getFileName();
      return name == null ? filePath : name.toString();
    } catch(RuntimeException e) {
      return filePath;
    }
  }

  // ---- AST navigation ---------------------------------------------

  /** Walk up to the nearest ancestor matching the given class (raw generic to match callers). */
  static <T> Optional<T> findAncestor(Node node, Class<T> type) {
    Optional<Node> parent = node.getParentNode();
    while(parent.isPresent()) {
      Node p = parent.get();
      if(type.isInstance(p)) {
        return Optional.of(type.cast(p));
      }
      parent = p.getParentNode();
    }
    return Optional.empty();
  }

  /**
   * The nearest enclosing type of a declaration, which may be an anonymous class body.
   *
   * <p>{@code findAncestor(node, TypeDeclaration.class)} walks straight past an anonymous body to
   * the named type outside it, so a member of an anonymous class would be attached to the wrong
   * owner -- and, on the paths that build names lexically, named after it too.
   */
  static Optional<Node> enclosingTypeOf(Node node) {
    for(Node cur = node.getParentNode().orElse(null); cur != null; cur = cur.getParentNode().orElse(null)) {
      if(cur instanceof TypeDeclaration) {
        return Optional.of(cur);
      }
      if(cur instanceof ObjectCreationExpr oce && oce.getAnonymousClassBody().isPresent()) {
        return Optional.of(cur);
      }
    }
    return Optional.empty();
  }

  /** The {@link ObjectCreationExpr} behind a resolved type, present only for anonymous classes. */
  private static Optional<ObjectCreationExpr> anonymousAstOf(ResolvedReferenceTypeDeclaration decl) {
    return decl.toAst().filter(ObjectCreationExpr.class::isInstance).map(ObjectCreationExpr.class::cast);
  }

  // ---- node kinds --------------------------------------------------

  static int kindFor(TypeDeclaration td) {
    if(td instanceof AnnotationDeclaration) {
      return Kinds.NODE_ANNOTATION;
    }
    if(td instanceof EnumDeclaration) {
      return Kinds.NODE_ENUM;
    }
    if(td instanceof ClassOrInterfaceDeclaration coif) {
      return coif.isInterface() ? Kinds.NODE_INTERFACE : Kinds.NODE_CLASS;
    }
    return Kinds.NODE_CLASS;
  }

  static int kindFor(ResolvedReferenceTypeDeclaration decl) {
    if(decl instanceof ResolvedAnnotationDeclaration) {
      return Kinds.NODE_ANNOTATION;
    }
    if(decl.isEnum()) {
      return Kinds.NODE_ENUM;
    }
    if(decl.isInterface()) {
      return Kinds.NODE_INTERFACE;
    }
    return Kinds.NODE_CLASS;
  }

  /** Node kind for a fully qualified type name, or 0 when the solver cannot place it. */
  int resolveKindOf(String qualifiedName) {
    try {
      SymbolReference<ResolvedReferenceTypeDeclaration> ref = typeSolver.tryToSolveType(qualifiedName);
      if(ref.isSolved()) {
        return kindFor(ref.getCorrespondingDeclaration());
      }
    } catch(RuntimeException e) {
      // Unresolvable name; the caller falls back to NODE_CLASS and marks the location unsolved.
    }
    return 0;
  }

  // ---- types -------------------------------------------------------

  /** Resolved name of a written type, falling back to the as-written form. */
  static String describe(Type type) {
    try {
      return type.resolve().describe();
    } catch(RuntimeException e) {
      return type.asString();
    }
  }

  private static String describe(ResolvedMethodLikeDeclaration decl, int index) {
    try {
      return decl.getParam(index).getType().describe();
    } catch(RuntimeException e) {
      return "?";
    }
  }

  /** FQ-name element chain for a type declaration (outer to inner order). */
  Names.Element[] chainForType(TypeDeclaration td) {
    try {
      return chainForResolvedType(td.resolve());
    } catch(RuntimeException e) {
      // Fall through to the lexical form.
    }
    List<String> outers = new ArrayList<>();
    Node cur = td.getParentNode().orElse(null);
    while(cur != null) {
      if(cur instanceof TypeDeclaration<?> t) {
        outers.add(t.getNameAsString());
      }
      cur = cur.getParentNode().orElse(null);
    }
    StringBuilder sb = new StringBuilder(resolver.defaultPackage());
    if(sb.length() > 0) {
      sb.append('.');
    }
    for(int i = outers.size() - 1; i >= 0; i--) {
      sb.append(outers.get(i)).append('.');
    }
    sb.append(td.getNameAsString());
    return Names.split(sb.toString());
  }

  /**
   * FQ-name element chain for a resolved type, with anonymous classes given a stable name. Every
   * owner name goes through here, so a declaration and a reference to it still produce the
   * identical string.
   *
   * <p>An anonymous class is recognised by its AST node being an {@link ObjectCreationExpr} rather
   * than a {@link TypeDeclaration}; the solver's own {@code isAnonymousClass()} returns false for
   * them and {@code containerType()} throws, so neither can be used here.
   *
   * <p>ponytail: a *named* class declared inside an anonymous class body still takes the
   * getQualifiedName() branch, and that name embeds the random id. Rare enough to leave; fix by
   * building the whole chain from the AST if it ever matters.
   */
  Names.Element[] chainForResolvedType(ResolvedReferenceTypeDeclaration decl) {
    if(decl == null) {
      return new Names.Element[0];
    }
    Optional<ObjectCreationExpr> anonymous = anonymousAstOf(decl);
    if(anonymous.isEmpty()) {
      String qualified = decl.getQualifiedName();
      return (qualified == null || qualified.isEmpty()) ? new Names.Element[0] : Names.split(qualified);
    }
    return chainForAnonymous(anonymous.get());
  }

  /** Element chain for an anonymous class body: its enclosing named type, then its positional name. */
  Names.Element[] chainForAnonymous(ObjectCreationExpr oce) {
    Names.Element[] container = findAncestor(oce, TypeDeclaration.class)
        .map(this::chainForType)
        .orElseGet(() -> new Names.Element[0]);
    return Names.concat(container, Names.Element.plain(anonymousTypeName(oce)));
  }

  /** Element chain for whatever {@link #enclosingTypeOf} returned. */
  Names.Element[] chainForEnclosingType(Node owner) {
    return (owner instanceof TypeDeclaration<?> td) ? chainForType(td) : chainForAnonymous((ObjectCreationExpr) owner);
  }

  /**
   * A stable name for an anonymous class.
   *
   * <p>JavaParser's symbol solver names them {@code Anonymous-<random UUID>}, a fresh value on
   * every run. The engine dedups nodes on ({@code type}, {@code serializedName}) at merge time, so
   * a random id means an anonymous class -- and every method and field inside it -- never merges
   * with itself, neither across the files that reference it nor across re-indexes of the same
   * file. Name it by source position instead, following the C++ indexer's convention in
   * {@code CxxDeclNameResolver::getNameForAnonymousSymbol}.
   */
  String anonymousTypeName(ObjectCreationExpr expr) {
    Optional<Range> range = expr.getRange();
    if(range.isEmpty()) {
      return "anonymous class";
    }
    Range r = range.get();
    return "anonymous class (" + fileName + "<" + r.begin.line + ":" + r.begin.column + ">)";
  }

  // ---- callables ---------------------------------------------------

  /**
   * Serialized name of a resolved method or constructor. Declarations and call sites both go through
   * this, so the two sides produce the identical string and merge into one node across files.
   */
  String chainForResolvedCallable(ResolvedMethodLikeDeclaration decl) {
    ResolvedReferenceTypeDeclaration owner = decl.declaringType();
    Names.Element[] parent = chainForResolvedType(owner);
    // A constructor is named after its type, so an anonymous one would carry the random id here
    // even though the owner chain above no longer does.
    String name = (decl instanceof ResolvedConstructorDeclaration)
        ? anonymousAstOf(owner).map(this::anonymousTypeName).orElseGet(decl::getName)
        : decl.getName();
    String returnType = "void";
    if(decl instanceof ResolvedMethodDeclaration method) {
      try {
        returnType = method.getReturnType().describe();
      } catch(RuntimeException e) {
        returnType = "void";
      }
    }
    StringBuilder params = new StringBuilder();
    for(int i = 0; i < decl.getNumberOfParams(); i++) {
      if(i > 0) {
        params.append(", ");
      }
      params.append(describe(decl, i));
    }
    return Names.join(Names.concat(parent, Names.Element.signature(name, returnType, "(" + params + ")")));
  }

  /**
   * Serialized name for a declared method or constructor. Resolves first, so the declaration lands
   * on exactly the string {@link #chainForResolvedCallable} produces at every call site; falls back
   * to the as-written types when the solver cannot resolve the declaration.
   */
  String serializedCallableName(CallableDeclaration<?> cd, String declaredReturnType) {
    try {
      ResolvedMethodLikeDeclaration resolved = (cd instanceof MethodDeclaration md)
          ? md.resolve()
          : ((ConstructorDeclaration) cd).resolve();
      return chainForResolvedCallable(resolved);
    } catch(RuntimeException e) {
      // Fall through to the lexical form.
    }

    NodeList<Parameter> params = cd.getParameters();
    StringBuilder joined = new StringBuilder();
    for(int i = 0; i < params.size(); i++) {
      if(i > 0) {
        joined.append(", ");
      }
      Parameter p = params.get(i);
      joined.append(p.getType().asString());
      if(p.isVarArgs()) {
        joined.append("[]");
      }
    }
    Names.Element[] parent = enclosingTypeOf(cd).map(this::chainForEnclosingType)
        .orElseGet(() -> new Names.Element[0]);
    return Names.join(
        Names.concat(parent, Names.Element.signature(cd.getNameAsString(), declaredReturnType, "(" + joined + ")")));
  }

  /** Whether {@code self} overrides {@code candidate}: same name, same erased parameter list. */
  static boolean overrides(ResolvedMethodDeclaration self, ResolvedMethodDeclaration candidate) {
    if(!self.getName().equals(candidate.getName()) || self.getNumberOfParams() != candidate.getNumberOfParams()) {
      return false;
    }
    for(int i = 0; i < self.getNumberOfParams(); i++) {
      if(!describe(self, i).equals(describe(candidate, i))) {
        return false;
      }
    }
    return true;
  }

  // ---- type parameters ---------------------------------------------

  /** The resolved form of a declared type parameter, via its owner: TypeParameter.resolve() throws. */
  static Optional<ResolvedTypeParameterDeclaration> resolveTypeParameter(TypeParameter tp) {
    Node parent = tp.getParentNode().orElse(null);
    try {
      List<ResolvedTypeParameterDeclaration> declared;
      if(parent instanceof TypeDeclaration<?> td) {
        declared = td.resolve().getTypeParameters();
      } else if(parent instanceof MethodDeclaration md) {
        declared = md.resolve().getTypeParameters();
      } else if(parent instanceof ConstructorDeclaration cd) {
        declared = cd.resolve().getTypeParameters();
      } else {
        return Optional.empty();
      }
      return declared.stream().filter(d -> tp.getNameAsString().equals(d.getName())).findFirst();
    } catch(RuntimeException e) {
      return Optional.empty();
    }
  }

  /** Element chain for a type parameter: its container, then its own name. */
  static Names.Element[] chainForTypeParameter(ResolvedTypeParameterDeclaration tp) {
    String container = tp.getContainerQualifiedName();
    if(container == null || container.isEmpty()) {
      return new Names.Element[]{Names.Element.plain(tp.getName())};
    }
    return Names.concat(splitTypeParameterContainer(container), Names.Element.plain(tp.getName()));
  }

  /**
   * Split a type parameter's container into name elements. A method container carries its parameter
   * list -- {@code p.Box.pick(U, java.lang.String)} -- and the dots inside that list are not name
   * separators, so only the part ahead of it is treated as a dotted path.
   */
  private static Names.Element[] splitTypeParameterContainer(String container) {
    int paren = container.indexOf('(');
    int dot = container.lastIndexOf('.', paren < 0 ? container.length() - 1 : paren);
    if(dot < 0) {
      return new Names.Element[]{Names.Element.plain(container)};
    }
    return Names.concat(Names.split(container.substring(0, dot)), Names.Element.plain(container.substring(dot + 1)));
  }

  // ---- local symbols -----------------------------------------------

  /**
   * The C++ convention from {@code CxxAstVisitorComponentIndexer::getLocalSymbolName}: the file name
   * plus the declaration's start position. Uses at other positions resolve back to this name, so one
   * variable is one symbol however many times it is read. Null when the node has no source range.
   */
  String localSymbolName(Node declaration) {
    Optional<Range> range = declaration.getRange();
    if(range.isEmpty()) {
      return null;
    }
    Range r = range.get();
    return fileName + "<" + r.begin.line + ":" + r.begin.column + ">";
  }
}
