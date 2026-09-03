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
import com.github.javaparser.ast.body.CompactConstructorDeclaration;
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
import com.github.javaparser.ast.expr.MethodReferenceExpr;
import com.github.javaparser.ast.expr.NameExpr;
import com.github.javaparser.ast.expr.ObjectCreationExpr;
import com.github.javaparser.ast.expr.TypeExpr;
import com.github.javaparser.ast.expr.VariableDeclarationExpr;
import com.github.javaparser.ast.nodeTypes.NodeWithAnnotations;
import com.github.javaparser.ast.nodeTypes.modifiers.NodeWithAccessModifiers;
import com.github.javaparser.ast.type.ClassOrInterfaceType;
import com.github.javaparser.ast.type.ReferenceType;
import com.github.javaparser.ast.type.Type;
import com.github.javaparser.ast.type.TypeParameter;
import com.github.javaparser.ast.visitor.VoidVisitorAdapter;
import com.github.javaparser.resolution.TypeSolver;
import com.github.javaparser.resolution.declarations.ResolvedMethodDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedReferenceTypeDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedTypeParameterDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedValueDeclaration;
import com.github.javaparser.resolution.types.ResolvedReferenceType;
import com.github.javaparser.resolution.types.ResolvedType;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.Optional;
import java.util.function.Supplier;

/**
 * Walks a parsed {@link CompilationUnit} and emits name / symbol / edge / location entries into a
 * {@link Storage} emitter. Every serialized name and node kind it emits comes from
 * {@link SymbolNames}; this class decides only <em>what</em> to emit and from which scope.
 *
 * <p>Reference edges originate from the enclosing symbol (the method, or failing that the type),
 * not from the file node - that is what makes the call graph navigable.
 */
public final class JavaCollector extends VoidVisitorAdapter<Void> {
  private final Storage storage;
  private final NameResolver resolver;
  private final SymbolNames names;

  /** Enclosing symbol ids, innermost last. Empty means "file scope". */
  private final Deque<Long> scopes = new ArrayDeque<>();

  public JavaCollector(Storage storage, NameResolver resolver, TypeSolver typeSolver, String filePath) {
    this.storage = storage;
    this.resolver = resolver;
    this.names = new SymbolNames(resolver, typeSolver, filePath);
  }

  public void visitRoot(CompilationUnit cu) {
    cu.accept(this, null);
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
    // The node kind is part of the merge key, so guessing NODE_CLASS for an imported interface
    // splits that interface into two nodes instead of erroring. Ask the solver what it actually is.
    int kind = names.resolveKindOf(name);
    boolean solved = kind != 0;
    long target = storage.nodeByName(solved ? kind : Kinds.NODE_CLASS, Names.join(Names.split(name)));
    location(storage.edge(storage.fileId(), target, Kinds.EDGE_IMPORT), imp,
        solved ? Kinds.LOCATION_TOKEN : Kinds.LOCATION_UNSOLVED);
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
    for(Parameter component : rd.getParameters()) {
      emitRecordComponent(rd, typeId, component);
    }
    inScope(typeId, () -> super.visit(rd, arg));
  }

  /**
   * A record component declares a private final field and a public accessor method (JLS 8.10.3).
   * JavaParser models the component as a {@link Parameter}, so without this it falls through
   * {@link #visit(Parameter, Void)} and is recorded as a local variable -- leaving the record
   * itself with no members at all.
   *
   * <p>The accessor has no AST node of its own, so it is keyed by name alone: the component's
   * node is already claimed by the field, and {@link Storage#nodeFor} would hand that id back.
   */
  private void emitRecordComponent(RecordDeclaration rd, long typeId, Parameter component) {
    long id = storage.nodeFor(component, Kinds.NODE_FIELD,
        Names.concat(names.chainForType(rd), Names.Element.plain(component.getNameAsString())));
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    storage.access(id, Kinds.ACCESS_PRIVATE);
    location(id, component.getName(), Kinds.LOCATION_TOKEN);
    storage.edge(typeId, id, Kinds.EDGE_MEMBER);
    typeReference(id, component.getType(), component.getType(), Kinds.EDGE_TYPE_USAGE);
    annotations(component, id);

    long accessorId = storage.nodeByName(Kinds.NODE_METHOD,
        Names.join(Names.concat(names.chainForType(rd), Names.Element.signature(component.getNameAsString(),
            SymbolNames.describe(component.getType()), "()"))));
    storage.symbol(accessorId, Kinds.DEFINITION_IMPLICIT);
    storage.access(accessorId, Kinds.ACCESS_PUBLIC);
    location(accessorId, component.getName(), Kinds.LOCATION_TOKEN);
    storage.edge(typeId, accessorId, Kinds.EDGE_MEMBER);
  }

  /**
   * A compact constructor <em>is</em> the canonical constructor, so it has to carry the same name a
   * {@code new Point(...)} call site produces. Its own parameter list is always empty, so the
   * components supply it; {@code ccd.resolve()} does not answer for a compact constructor.
   */
  @Override
  public void visit(CompactConstructorDeclaration ccd, Void arg) {
    StringBuilder params = new StringBuilder();
    for(Parameter component : SymbolNames.findAncestor(ccd, RecordDeclaration.class)
        .map(RecordDeclaration::getParameters).orElseGet(NodeList::new)) {
      if(params.length() > 0) {
        params.append(", ");
      }
      params.append(SymbolNames.describe(component.getType()));
    }
    String name = Names.join(Names.concat(
        SymbolNames.enclosingTypeOf(ccd).map(names::chainForEnclosingType)
            .orElseGet(() -> new Names.Element[0]),
        Names.Element.signature(ccd.getNameAsString(), "void", "(" + params + ")")));
    long id = storage.nodeFor(ccd, Kinds.NODE_METHOD, name);
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(ccd, id);
    location(id, ccd.getName(), Kinds.LOCATION_TOKEN);
    location(id, ccd, Kinds.LOCATION_SCOPE);
    annotations(ccd, id);
    SymbolNames.enclosingTypeOf(ccd)
        .ifPresent(parent -> storage.edge(idOfEnclosingType(parent), id, Kinds.EDGE_MEMBER));
    inScope(id, () -> super.visit(ccd, arg));
  }

  @Override
  public void visit(EnumConstantDeclaration ecd, Void arg) {
    Optional<TypeDeclaration> parentOpt = SymbolNames.findAncestor(ecd, TypeDeclaration.class);
    if(parentOpt.isEmpty()) {
      return;
    }
    TypeDeclaration parent = parentOpt.get();
    Names.Element[] chain =
        Names.concat(names.chainForType(parent), Names.Element.plain(ecd.getNameAsString()));
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
    long id = storage.nodeFor(cd, Kinds.NODE_METHOD, names.serializedCallableName(cd, declaredReturnType));
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(cd, id);
    location(id, cd.getName(), Kinds.LOCATION_TOKEN);
    annotations(cd, id);
    SymbolNames.enclosingTypeOf(cd)
        .ifPresent(parent -> storage.edge(idOfEnclosingType(parent), id, Kinds.EDGE_MEMBER));
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
          if(SymbolNames.overrides(resolved, candidate)) {
            long overridden =
                storage.nodeByName(Kinds.NODE_METHOD, names.chainForResolvedCallable(candidate));
            // Located at the overriding method's own name, the way the C++ indexer records an
            // override reference: there is no other token in the file that stands for it.
            location(storage.edge(id, overridden, Kinds.EDGE_OVERRIDE), md.getName(), Kinds.LOCATION_TOKEN);
          }
        }
      }
    } catch(RuntimeException | StackOverflowError e) {
      // Cyclic or unresolvable ancestry: the override edges are a bonus, not worth failing over.
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
    Optional<Node> parentOpt = SymbolNames.enclosingTypeOf(var);
    Names.Element[] chain = parentOpt
        .map(parent -> Names.concat(names.chainForEnclosingType(parent),
            Names.Element.plain(var.getNameAsString())))
        .orElseGet(() -> new Names.Element[]{Names.Element.plain(var.getNameAsString())});

    long id = storage.nodeFor(var, Kinds.NODE_FIELD, chain);
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(fd, id);
    location(id, var.getName(), Kinds.LOCATION_TOKEN);
    parentOpt.ifPresent(parent -> storage.edge(idOfEnclosingType(parent), id, Kinds.EDGE_MEMBER));
    typeReference(id, var.getType(), var.getType(), Kinds.EDGE_TYPE_USAGE);
    annotations(fd, id);
  }

  // ---- local symbols -------------------------------------------

  @Override
  public void visit(Parameter param, Void arg) {
    // A record component is a field, emitted by visit(RecordDeclaration); it is not a local.
    if(param.getParentNode().filter(RecordDeclaration.class::isInstance).isPresent()) {
      super.visit(param, arg);
      return;
    }
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
    long id = emitTypeParameter(tp);
    for(ClassOrInterfaceType bound : tp.getTypeBound()) {
      typeReference(id != 0 ? id : scope(), bound, bound, Kinds.EDGE_TYPE_USAGE);
    }
    super.visit(tp, arg);
  }

  /**
   * A type parameter is a symbol of its own, not a local variable: uses of {@code T} inside the
   * class have to resolve back to it, or they fabricate a class node named {@code T} in the current
   * package -- the same defect
   * {@code JavaIndexerTest.a_bare_local_variable_read_does_not_fabricate_a_class_node} guards
   * against for variables.
   *
   * <p>The name has to match what {@link #typeReference} builds for a use, so both sides go through
   * {@link SymbolNames#chainForTypeParameter} on the same resolved declaration.
   */
  private long emitTypeParameter(TypeParameter tp) {
    Optional<ResolvedTypeParameterDeclaration> resolved = SymbolNames.resolveTypeParameter(tp);
    if(resolved.isEmpty()) {
      // Unresolvable owner: keep the old local-symbol form rather than inventing a name that no
      // use site could reproduce.
      long local = declareLocal(tp, tp.getName());
      if(local != 0) {
        storage.access(local, Kinds.ACCESS_TYPE_PARAMETER);
      }
      return 0;
    }

    long id = storage.nodeFor(tp, Kinds.NODE_TYPE_PARAMETER,
        SymbolNames.chainForTypeParameter(resolved.get()));
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    storage.access(id, Kinds.ACCESS_TYPE_PARAMETER);
    location(id, tp.getName(), Kinds.LOCATION_TOKEN);
    storage.edge(scope(), id, Kinds.EDGE_MEMBER);
    return id;
  }

  /** Record a local symbol at its declaration and return its id (0 if it has no source range). */
  private long declareLocal(Node declaration, Node nameNode) {
    String name = names.localSymbolName(declaration);
    if(name == null) {
      return 0;
    }
    long id = storage.localSymbol(name);
    location(id, nameNode, Kinds.LOCATION_LOCAL_SYMBOL);
    return id;
  }

  // ---- expressions -----------------------------------------------

  @Override
  public void visit(MethodCallExpr mce, Void arg) {
    String serialized = null;
    try {
      serialized = names.chainForResolvedCallable(mce.resolve());
    } catch(RuntimeException e) {
      // Unresolvable call: fall through to the name-only form below.
    }
    emitCall(serialized, mce.getNameAsString(), mce.getName());
    super.visit(mce, arg);
  }

  /**
   * A method reference is a call: {@code list.forEach(this::helper)} reaches {@code helper} just as
   * {@code helper()} does, and without this the call graph simply loses that edge.
   *
   * <p>The location covers the whole {@code scope::name} expression rather than just the name
   * token: JavaParser exposes the identifier as a String with no range of its own, and deriving one
   * from the expression's end would break on {@code Foo :: bar}.
   */
  @Override
  public void visit(MethodReferenceExpr mre, Void arg) {
    if("new".equals(mre.getIdentifier())) {
      // ponytail: JavaParser cannot resolve a constructor reference ("Constructor calls not yet
      //           resolvable"), so record the type being constructed and skip the call edge rather
      //           than fabricating a "new()" method node that matches no declaration.
      if(mre.getScope() instanceof TypeExpr te) {
        typeReference(scope(), te.getType(), te, Kinds.EDGE_TYPE_USAGE);
      }
      super.visit(mre, arg);
      return;
    }

    String serialized = null;
    try {
      serialized = names.chainForResolvedCallable(mre.resolve());
    } catch(RuntimeException e) {
      // Unresolvable reference: fall through to the name-only form below.
    }
    emitCall(serialized, mre.getIdentifier(), mre);
    super.visit(mre, arg);
  }

  /**
   * Emit a call edge from the current scope. A null {@code serialized} means the solver could not
   * place the callee: no owner and no parameter types are knowable, so the node can only ever be a
   * placeholder and the location is marked unsolved.
   */
  private void emitCall(String serialized, String calleeName, Node range) {
    boolean solved = serialized != null;
    if(!solved) {
      serialized = Names.join(Names.Element.signature(calleeName, "", "()"));
    }
    long target = storage.nodeByName(Kinds.NODE_METHOD, serialized);
    location(storage.edge(scope(), target, Kinds.EDGE_CALL), range,
        solved ? Kinds.LOCATION_TOKEN : Kinds.LOCATION_UNSOLVED);
  }

  @Override
  public void visit(ObjectCreationExpr oce, Void arg) {
    try {
      long ctor = storage.nodeByName(Kinds.NODE_METHOD, names.chainForResolvedCallable(oce.resolve()));
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
        // The solver's own declaringType() reports the nearest *named* class, so a field of an
        // anonymous class comes back owned by the class outside it -- and the read would then name
        // a different node than the declaration. Take the owner from the AST when there is one.
        Names.Element[] owner = resolved.toAst()
            .flatMap(SymbolNames::enclosingTypeOf)
            .map(names::chainForEnclosingType)
            .orElseGet(() -> names.chainForResolvedType(resolved.asField().declaringType().asReferenceType()));
        long target = storage.nodeByName(
            Kinds.NODE_FIELD, Names.join(Names.concat(owner, Names.Element.plain(resolved.getName()))));
        location(storage.edge(scope(), target, Kinds.EDGE_USAGE), range, Kinds.LOCATION_TOKEN);
        return;
      }
      Optional<Node> declaration = resolved.toAst();
      if(declaration.isPresent()) {
        String name = names.localSymbolName(declaration.get());
        if(name != null) {
          location(storage.localSymbol(name), range, Kinds.LOCATION_LOCAL_SYMBOL);
        }
      }
    } catch(RuntimeException e) {
      // Half-resolved symbol; emitting nothing beats emitting a wrong edge.
    }
  }

  // ---- emit helpers ----------------------------------------------

  private void inScope(long id, Runnable body) {
    scopes.push(id);
    try {
      body.run();
    } finally {
      scopes.pop();
    }
  }

  private long idOfType(TypeDeclaration td) {
    if(!storage.known(td)) {
      storage.nodeFor(td, SymbolNames.kindFor(td), names.chainForType(td));
    }
    return storage.idOf(td);
  }

  /** Node id for whatever {@link SymbolNames#enclosingTypeOf} returned, creating the node if needed. */
  private long idOfEnclosingType(Node owner) {
    if(owner instanceof TypeDeclaration<?> td) {
      return idOfType(td);
    }
    if(!storage.known(owner)) {
      storage.nodeFor(owner, Kinds.NODE_CLASS, names.chainForAnonymous((ObjectCreationExpr) owner));
    }
    return storage.idOf(owner);
  }

  private long emitType(TypeDeclaration td, int nodeType) {
    long id = storage.nodeFor(td, nodeType, names.chainForType(td));
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(td, id);
    location(id, td.getName(), Kinds.LOCATION_TOKEN);
    if(td.getMembers() != null && !td.getMembers().isEmpty()) {
      location(id, td, Kinds.LOCATION_SCOPE);
    }
    SymbolNames.findAncestor(td, TypeDeclaration.class)
        .ifPresent(outer -> storage.edge(idOfType(outer), id, Kinds.EDGE_MEMBER));
    return id;
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
      if(resolvedType.isTypeVariable()) {
        // A use of a type parameter; without this it falls through to the lexical form below and
        // fabricates a class node named after the parameter in the current package.
        long parameter = storage.nodeByName(Kinds.NODE_TYPE_PARAMETER, Names.join(
            SymbolNames.chainForTypeParameter(resolvedType.asTypeVariable().asTypeParameter())));
        location(storage.edge(from, parameter, edgeType), range, Kinds.LOCATION_TOKEN);
        return;
      }
      if(resolvedType.isReferenceType()) {
        ResolvedReferenceType resolved = resolvedType.asReferenceType();
        fqn = resolved.getQualifiedName();
        Optional<ResolvedReferenceTypeDeclaration> decl = resolved.getTypeDeclaration();
        if(decl.isPresent()) {
          kind = SymbolNames.kindFor(decl.get());
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

  private void location(long id, Node n, int type) {
    Optional<Range> range = n.getRange();
    if(range.isEmpty()) {
      return;
    }
    Range r = range.get();
    storage.location(id, r.begin.line, r.begin.column, r.end.line, r.end.column, type);
  }
}
