package com.sourcetrail.indexer;

import com.github.javaparser.Range;
import com.github.javaparser.ast.CompilationUnit;
import com.github.javaparser.ast.ImportDeclaration;
import com.github.javaparser.ast.Node;
import com.github.javaparser.ast.NodeList;
import com.github.javaparser.ast.PackageDeclaration;
import com.github.javaparser.ast.body.AnnotationDeclaration;
import com.github.javaparser.ast.body.CallableDeclaration;
import com.github.javaparser.ast.body.ClassOrInterfaceDeclaration;
import com.github.javaparser.ast.body.ConstructorDeclaration;
import com.github.javaparser.ast.body.EnumConstantDeclaration;
import com.github.javaparser.ast.body.EnumDeclaration;
import com.github.javaparser.ast.body.FieldDeclaration;
import com.github.javaparser.ast.body.MethodDeclaration;
import com.github.javaparser.ast.body.Parameter;
import com.github.javaparser.ast.body.RecordDeclaration;
import com.github.javaparser.ast.body.TypeDeclaration;
import com.github.javaparser.ast.body.VariableDeclarator;
import com.github.javaparser.ast.expr.AnnotationExpr;
import com.github.javaparser.ast.expr.FieldAccessExpr;
import com.github.javaparser.ast.expr.MethodCallExpr;
import com.github.javaparser.ast.expr.NameExpr;
import com.github.javaparser.ast.expr.ObjectCreationExpr;
import com.github.javaparser.ast.expr.VariableDeclarationExpr;
import com.github.javaparser.ast.nodeTypes.NodeWithAnnotations;
import com.github.javaparser.ast.nodeTypes.modifiers.NodeWithAccessModifiers;
import com.github.javaparser.ast.type.ClassOrInterfaceType;
import com.github.javaparser.ast.type.ReferenceType;
import com.github.javaparser.ast.type.Type;
import com.github.javaparser.ast.type.TypeParameter;
import com.github.javaparser.ast.visitor.VoidVisitorAdapter;
import com.github.javaparser.resolution.declarations.ResolvedAnnotationDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedMethodDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedMethodLikeDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedReferenceTypeDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedValueDeclaration;
import com.github.javaparser.resolution.types.ResolvedReferenceType;
import com.github.javaparser.resolution.types.ResolvedType;
import java.nio.file.Path;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.Optional;
import java.util.function.Supplier;

/**
 * Walks a parsed {@link CompilationUnit} and emits name / symbol / edge /
 * location entries into a {@link Storage} emitter.
 *
 * <p>Node identity for the engine merge step is ({@code type, serializedName}); the C++ side
 * re-hydrates that name on every lookup and dedups nodes by it at merge time. Names therefore come
 * from the <em>resolved</em> declaration wherever the symbol solver can supply one, so that a
 * declaration in one file and a reference to it in another produce the identical string. When
 * resolution fails the lexical {@link NameResolver} form is used instead and the reference's
 * location is marked {@link Kinds#LOCATION_UNSOLVED}.
 *
 * <p>Reference edges originate from the enclosing symbol (the method, or failing that the type),
 * not from the file node - that is what makes the call graph navigable.
 */
public final class JavaCollector extends VoidVisitorAdapter<Void> {
  private final Storage storage;
  private final NameResolver resolver;
  private final String fileName;

  /** Enclosing symbol ids, innermost last. Empty means "file scope". */
  private final Deque<Long> scopes = new ArrayDeque<>();

  public JavaCollector(Storage storage, NameResolver resolver, String filePath) {
    this.storage = storage;
    this.resolver = resolver;
    this.fileName = fileNameOf(filePath);
  }

  public void visitRoot(CompilationUnit cu) {
    cu.accept(this, null);
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

  /** The symbol a reference belongs to: innermost method, else innermost type, else the file. */
  private long scope() {
    Long top = scopes.peek();
    return top == null ? storage.fileId() : top;
  }

  // ---- package + imports ------------------------------------------

  @Override
  public void visit(PackageDeclaration pkg, Void arg) {
    String pkgName = pkg.getNameAsString();
    if(pkgName == null || pkgName.isEmpty()) {
      return;
    }
    long id = storage.nodeFor(pkg, Kinds.NODE_PACKAGE, Names.split(pkgName));
    location(id, pkg, Kinds.LOCATION_TOKEN);
    super.visit(pkg, arg);
  }

  @Override
  public void visit(ImportDeclaration imp, Void arg) {
    // A wildcard import names a package, not a symbol; NameResolver already ignores them and an
    // edge to "java.util.*" would only add a node nothing else ever references.
    if(imp.isAsterisk()) {
      return;
    }
    String name = imp.getNameAsString();
    if(name == null || name.isEmpty()) {
      return;
    }
    long target = storage.nodeByName(Kinds.NODE_CLASS, Names.join(Names.split(name)));
    location(storage.edge(storage.fileId(), target, Kinds.EDGE_IMPORT), imp, Kinds.LOCATION_TOKEN);
    super.visit(imp, arg);
  }

  // ---- type declarations ------------------------------------------

  @Override
  public void visit(ClassOrInterfaceDeclaration coif, Void arg) {
    long typeId = emitType(coif, coif.isInterface() ? Kinds.NODE_INTERFACE : Kinds.NODE_CLASS);
    annotations(coif, typeId);
    for(ClassOrInterfaceType ext : coif.getExtendedTypes()) {
      typeReference(typeId, ext, ext, Kinds.EDGE_INHERITANCE);
    }
    for(ClassOrInterfaceType impl : coif.getImplementedTypes()) {
      typeReference(typeId, impl, impl, Kinds.EDGE_INHERITANCE);
    }
    inScope(typeId, () -> super.visit(coif, arg));
  }

  @Override
  public void visit(EnumDeclaration ed, Void arg) {
    long typeId = emitType(ed, Kinds.NODE_ENUM);
    annotations(ed, typeId);
    for(ClassOrInterfaceType impl : ed.getImplementedTypes()) {
      typeReference(typeId, impl, impl, Kinds.EDGE_INHERITANCE);
    }
    inScope(typeId, () -> super.visit(ed, arg));
  }

  @Override
  public void visit(AnnotationDeclaration ad, Void arg) {
    long typeId = emitType(ad, Kinds.NODE_ANNOTATION);
    inScope(typeId, () -> super.visit(ad, arg));
  }

  @Override
  public void visit(RecordDeclaration rd, Void arg) {
    long typeId = emitType(rd, Kinds.NODE_CLASS);
    annotations(rd, typeId);
    for(ClassOrInterfaceType impl : rd.getImplementedTypes()) {
      typeReference(typeId, impl, impl, Kinds.EDGE_INHERITANCE);
    }
    inScope(typeId, () -> super.visit(rd, arg));
  }

  @Override
  public void visit(EnumConstantDeclaration ecd, Void arg) {
    Optional<TypeDeclaration> parentOpt = findAncestor(ecd, TypeDeclaration.class);
    if(parentOpt.isEmpty()) {
      return;
    }
    TypeDeclaration parent = parentOpt.get();
    Names.Element[] chain = concat(chainForType(parent), Names.Element.plain(ecd.getNameAsString()));
    long id = storage.nodeFor(ecd, Kinds.NODE_ENUM_CONSTANT, chain);
    storage.edge(idOfType(parent), id, Kinds.EDGE_MEMBER);
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    location(id, ecd, Kinds.LOCATION_TOKEN);
    super.visit(ecd, arg);
  }

  // ---- methods + constructors -----------------------------------

  @Override
  public void visit(MethodDeclaration md, Void arg) {
    long id = emitCallable(md, md.getType().asString());
    if(md.getBody().isPresent()) {
      location(id, md, Kinds.LOCATION_SCOPE);
    }
    for(ReferenceType thrown : md.getThrownExceptions()) {
      typeReference(id, thrown, thrown, Kinds.EDGE_TYPE_USAGE);
    }
    recordOverrides(md, id);
    inScope(id, () -> super.visit(md, arg));
  }

  @Override
  public void visit(ConstructorDeclaration cd, Void arg) {
    long id = emitCallable(cd, "void");
    location(id, cd, Kinds.LOCATION_SCOPE);
    inScope(id, () -> super.visit(cd, arg));
  }

  private long emitCallable(CallableDeclaration<?> cd, String declaredReturnType) {
    long id = storage.nodeFor(cd, Kinds.NODE_METHOD, serializedCallableName(cd, declaredReturnType));
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(cd, id);
    location(id, cd.getName(), Kinds.LOCATION_TOKEN);
    annotations(cd, id);
    findAncestor(cd, TypeDeclaration.class).ifPresent(parent -> storage.edge(idOfType(parent), id, Kinds.EDGE_MEMBER));
    return id;
  }

  /**
   * Emit EDGE_OVERRIDE for every ancestor method this one overrides or implements. Needs the symbol
   * solver: without an ancestor chain there is nothing to match against, so this is silently a no-op
   * on an unresolvable type.
   */
  private void recordOverrides(MethodDeclaration md, long id) {
    ResolvedMethodDeclaration resolved;
    try {
      resolved = md.resolve();
    } catch(RuntimeException e) {
      return;
    }
    try {
      for(ResolvedReferenceType ancestor : resolved.declaringType().getAllAncestors()) {
        Optional<ResolvedReferenceTypeDeclaration> decl = ancestor.getTypeDeclaration();
        if(decl.isEmpty()) {
          continue;
        }
        for(ResolvedMethodDeclaration candidate : decl.get().getDeclaredMethods()) {
          if(overrides(resolved, candidate)) {
            storage.edge(id, storage.nodeByName(Kinds.NODE_METHOD, chainForResolvedCallable(candidate)),
                Kinds.EDGE_OVERRIDE);
          }
        }
      }
    } catch(RuntimeException | StackOverflowError e) {
      // Cyclic or unresolvable ancestry: the override edges are a bonus, not worth failing over.
    }
  }

  private static boolean overrides(ResolvedMethodDeclaration self, ResolvedMethodDeclaration candidate) {
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

  private static String describe(ResolvedMethodLikeDeclaration decl, int index) {
    try {
      return decl.getParam(index).getType().describe();
    } catch(RuntimeException e) {
      return "?";
    }
  }

  // ---- fields ---------------------------------------------------------

  @Override
  public void visit(FieldDeclaration fd, Void arg) {
    for(VariableDeclarator var : fd.getVariables()) {
      emitField(fd, var);
    }
    super.visit(fd, arg);
  }

  private void emitField(FieldDeclaration fd, VariableDeclarator var) {
    Optional<TypeDeclaration> parentOpt = findAncestor(var, TypeDeclaration.class);
    Names.Element[] chain = parentOpt
        .map(parent -> concat(chainForType(parent), Names.Element.plain(var.getNameAsString())))
        .orElseGet(() -> new Names.Element[]{Names.Element.plain(var.getNameAsString())});

    long id = storage.nodeFor(var, Kinds.NODE_FIELD, chain);
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(fd, id);
    location(id, var.getName(), Kinds.LOCATION_TOKEN);
    parentOpt.ifPresent(parent -> storage.edge(idOfType(parent), id, Kinds.EDGE_MEMBER));
    typeReference(id, var.getType(), var.getType(), Kinds.EDGE_TYPE_USAGE);
    annotations(fd, id);
  }

  // ---- local symbols -------------------------------------------

  @Override
  public void visit(Parameter param, Void arg) {
    declareLocal(param, param.getName());
    typeReference(scope(), param.getType(), param.getType(), Kinds.EDGE_TYPE_USAGE);
    super.visit(param, arg);
  }

  @Override
  public void visit(VariableDeclarationExpr vde, Void arg) {
    for(VariableDeclarator var : vde.getVariables()) {
      declareLocal(var, var.getName());
      typeReference(scope(), var.getType(), var.getType(), Kinds.EDGE_TYPE_USAGE);
    }
    super.visit(vde, arg);
  }

  @Override
  public void visit(TypeParameter tp, Void arg) {
    long id = declareLocal(tp, tp.getName());
    for(ClassOrInterfaceType bound : tp.getTypeBound()) {
      typeReference(scope(), bound, bound, Kinds.EDGE_TYPE_USAGE);
    }
    if(id != 0) {
      storage.access(id, Kinds.ACCESS_TYPE_PARAMETER);
    }
    super.visit(tp, arg);
  }

  /** Record a local symbol at its declaration and return its id (0 if it has no source range). */
  private long declareLocal(Node declaration, Node nameNode) {
    String name = localSymbolName(declaration);
    if(name == null) {
      return 0;
    }
    long id = storage.localSymbol(name);
    location(id, nameNode, Kinds.LOCATION_LOCAL_SYMBOL);
    return id;
  }

  /**
   * The C++ convention from {@code CxxAstVisitorComponentIndexer::getLocalSymbolName}: the file name
   * plus the declaration's start position. Uses at other positions resolve back to this name, so one
   * variable is one symbol however many times it is read.
   */
  private String localSymbolName(Node declaration) {
    Optional<Range> range = declaration.getRange();
    if(range.isEmpty()) {
      return null;
    }
    Range r = range.get();
    return fileName + "<" + r.begin.line + ":" + r.begin.column + ">";
  }

  // ---- expressions -----------------------------------------------

  @Override
  public void visit(MethodCallExpr mce, Void arg) {
    String serialized = null;
    try {
      serialized = chainForResolvedCallable(mce.resolve());
    } catch(RuntimeException e) {
      // Unresolvable call: fall through to the name-only form below.
    }

    boolean solved = serialized != null;
    if(!solved) {
      // No owner and no parameter types are knowable, so this node can only ever be a placeholder.
      serialized = Names.join(Names.Element.signature(mce.getNameAsString(), "", "()"));
    }

    long target = storage.nodeByName(Kinds.NODE_METHOD, serialized);
    location(storage.edge(scope(), target, Kinds.EDGE_CALL), mce.getName(),
        solved ? Kinds.LOCATION_TOKEN : Kinds.LOCATION_UNSOLVED);
    super.visit(mce, arg);
  }

  @Override
  public void visit(ObjectCreationExpr oce, Void arg) {
    try {
      long ctor = storage.nodeByName(Kinds.NODE_METHOD, chainForResolvedCallable(oce.resolve()));
      location(storage.edge(scope(), ctor, Kinds.EDGE_CALL), oce.getType(), Kinds.LOCATION_TOKEN);
    } catch(RuntimeException e) {
      typeReference(scope(), oce.getType(), oce.getType(), Kinds.EDGE_TYPE_USAGE);
    }
    super.visit(oce, arg);
  }

  @Override
  public void visit(FieldAccessExpr fae, Void arg) {
    valueReference(fae::resolve, fae.getName());
    super.visit(fae, arg);
  }

  @Override
  public void visit(NameExpr ne, Void arg) {
    // Deliberately no lexical fallback here. A bare name is usually a local variable or a field,
    // and guessing "it is a class in the current package" - which is what NameResolver.isResolvable
    // does whenever a package declaration exists - fabricates a node for every variable read.
    valueReference(ne::resolve, ne);
    super.visit(ne, arg);
  }

  /** Emit a usage of a resolved field, or an occurrence of a resolved local variable/parameter. */
  private void valueReference(Supplier<ResolvedValueDeclaration> resolver, Node range) {
    try {
      ResolvedValueDeclaration resolved = resolver.get();
      if(resolved == null) {
        return;
      }
      if(resolved.isField()) {
        String owner = resolved.asField().declaringType().getQualifiedName();
        long target = storage.nodeByName(Kinds.NODE_FIELD, Names.join(Names.split(owner + "." + resolved.getName())));
        location(storage.edge(scope(), target, Kinds.EDGE_USAGE), range, Kinds.LOCATION_TOKEN);
        return;
      }
      Optional<Node> declaration = resolved.toAst();
      if(declaration.isPresent()) {
        String name = localSymbolName(declaration.get());
        if(name != null) {
          location(storage.localSymbol(name), range, Kinds.LOCATION_LOCAL_SYMBOL);
        }
      }
    } catch(RuntimeException e) {
      // Half-resolved symbol; emitting nothing beats emitting a wrong edge.
    }
  }

  // ---- helpers ---------------------------------------------------

  private void inScope(long id, Runnable body) {
    scopes.push(id);
    try {
      body.run();
    } finally {
      scopes.pop();
    }
  }

  /** Walk up to the nearest ancestor matching the given class (raw generic to match callers). */
  private static <T> Optional<T> findAncestor(Node node, Class<T> type) {
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

  private long idOfType(TypeDeclaration td) {
    if(!storage.known(td)) {
      storage.nodeFor(td, kindFor(td), chainForType(td));
    }
    return storage.idOf(td);
  }

  private long emitType(TypeDeclaration td, int nodeType) {
    long id = storage.nodeFor(td, nodeType, chainForType(td));
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(td, id);
    location(id, td.getName(), Kinds.LOCATION_TOKEN);
    if(td.getMembers() != null && !td.getMembers().isEmpty()) {
      location(id, td, Kinds.LOCATION_SCOPE);
    }
    findAncestor(td, TypeDeclaration.class).ifPresent(outer -> storage.edge(idOfType(outer), id, Kinds.EDGE_MEMBER));
    return id;
  }

  private int kindFor(TypeDeclaration td) {
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

  private void access(NodeWithAccessModifiers<?> node, long id) {
    if(node.isPublic()) {
      storage.access(id, Kinds.ACCESS_PUBLIC);
    } else if(node.isPrivate()) {
      storage.access(id, Kinds.ACCESS_PRIVATE);
    } else if(node.isProtected()) {
      storage.access(id, Kinds.ACCESS_PROTECTED);
    } else {
      storage.access(id, Kinds.ACCESS_DEFAULT);
    }
  }

  private void annotations(NodeWithAnnotations<?> node, long nodeId) {
    for(AnnotationExpr ann : node.getAnnotations()) {
      String fqn = null;
      try {
        fqn = ann.resolve().getQualifiedName();
      } catch(RuntimeException e) {
        fqn = null;
      }
      boolean solved = fqn != null;
      if(!solved) {
        fqn = resolver.resolveName(ann.getNameAsString());
      }
      if(fqn == null || fqn.isEmpty()) {
        continue;
      }
      long target = storage.nodeByName(Kinds.NODE_ANNOTATION, Names.join(Names.split(fqn)));
      location(storage.edge(nodeId, target, Kinds.EDGE_ANNOTATION_USAGE), ann,
          solved ? Kinds.LOCATION_TOKEN : Kinds.LOCATION_UNSOLVED);
    }
  }

  /**
   * Emit a reference to a type. Resolved references carry the qualified name and the real node kind
   * (a reference to an interface must be NODE_INTERFACE, or it will not merge with the interface's
   * own declaration); unresolved ones fall back to the lexical name, NODE_CLASS, and an UNSOLVED
   * location marker.
   */
  private void typeReference(long from, Type type, Node range, int edgeType) {
    if(!(type instanceof ClassOrInterfaceType cit)) {
      return;
    }
    String fqn = null;
    int kind = Kinds.NODE_CLASS;
    try {
      ResolvedType resolvedType = cit.resolve();
      if(resolvedType.isReferenceType()) {
        ResolvedReferenceType resolved = resolvedType.asReferenceType();
        fqn = resolved.getQualifiedName();
        Optional<ResolvedReferenceTypeDeclaration> decl = resolved.getTypeDeclaration();
        if(decl.isPresent()) {
          kind = kindFor(decl.get());
        }
      }
    } catch(RuntimeException e) {
      fqn = null;
    }

    boolean solved = fqn != null && !fqn.isEmpty();
    if(!solved) {
      fqn = resolver.resolveType(cit);
    }
    if(fqn == null || fqn.isEmpty()) {
      return;
    }

    long target = storage.nodeByName(kind, Names.join(Names.split(fqn)));
    location(storage.edge(from, target, edgeType), range, solved ? Kinds.LOCATION_TOKEN : Kinds.LOCATION_UNSOLVED);
  }

  private static int kindFor(ResolvedReferenceTypeDeclaration decl) {
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

  private void location(long id, Node n, int type) {
    Optional<Range> range = n.getRange();
    if(range.isEmpty()) {
      return;
    }
    Range r = range.get();
    storage.location(id, r.begin.line, r.begin.column, r.end.line, r.end.column, type);
  }

  /** FQ-name element chain for a type declaration (outer to inner order). */
  private Names.Element[] chainForType(TypeDeclaration td) {
    try {
      return Names.split(td.resolve().getQualifiedName());
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
   * Serialized name of a resolved method or constructor. Declarations and call sites both go through
   * this, so the two sides produce the identical string and merge into one node across files.
   */
  private static String chainForResolvedCallable(ResolvedMethodLikeDeclaration decl) {
    String owner = decl.declaringType().getQualifiedName();
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
    Names.Element[] parent = (owner == null || owner.isEmpty()) ? new Names.Element[0] : Names.split(owner);
    return Names.join(concat(parent, Names.Element.signature(decl.getName(), returnType, "(" + params + ")")));
  }

  /**
   * Serialized name for a declared method or constructor. Resolves first, so the declaration lands
   * on exactly the string {@link #chainForResolvedCallable} produces at every call site; falls back
   * to the as-written types when the solver cannot resolve the declaration.
   */
  private String serializedCallableName(CallableDeclaration<?> cd, String declaredReturnType) {
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
    Names.Element[] parent = findAncestor(cd, TypeDeclaration.class).map(this::chainForType)
        .orElseGet(() -> new Names.Element[0]);
    return Names.join(
        concat(parent, Names.Element.signature(cd.getNameAsString(), declaredReturnType, "(" + joined + ")")));
  }

  private static Names.Element[] concat(Names.Element[] base, Names.Element extra) {
    Names.Element[] out = new Names.Element[base.length + 1];
    System.arraycopy(base, 0, out, 0, base.length);
    out[base.length] = extra;
    return out;
  }
}
