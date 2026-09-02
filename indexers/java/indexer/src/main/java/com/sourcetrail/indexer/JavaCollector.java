package com.sourcetrail.indexer;

import com.github.javaparser.Range;
import com.github.javaparser.ast.CompilationUnit;
import com.github.javaparser.ast.ImportDeclaration;
import com.github.javaparser.ast.Node;
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
import com.github.javaparser.ast.expr.MethodCallExpr;
import com.github.javaparser.ast.expr.NameExpr;
import com.github.javaparser.ast.expr.ObjectCreationExpr;
import com.github.javaparser.ast.nodeTypes.NodeWithAnnotations;
import com.github.javaparser.ast.nodeTypes.modifiers.NodeWithAccessModifiers;
import com.github.javaparser.ast.type.ClassOrInterfaceType;
import com.github.javaparser.ast.type.ReferenceType;
import com.github.javaparser.ast.type.TypeParameter;
import com.github.javaparser.ast.visitor.VoidVisitorAdapter;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

/**
 * Walks a parsed {@link CompilationUnit} and emits name / symbol / edge /
 * location entries into a {@link Storage} emitter.
 *
 * <p>Node identity for the engine merge step is ({@code type, serializedName});
 * the C++ side re-hydrates that name on every lookup and dedups nodes by it at
 * merge time. For {@code class C} in package {@code a.b} the chain is
 * {@code a, b, C}.
 */
public final class JavaCollector extends VoidVisitorAdapter<Void> {
  private final Storage storage;
  private final NameResolver resolver;

  public JavaCollector(Storage storage, NameResolver resolver) {
    this.storage = storage;
    this.resolver = resolver;
  }

  public void visitRoot(CompilationUnit cu) {
    cu.accept(this, null);
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
    String name = imp.getNameAsString();
    if(name == null || name.isEmpty()) {
      return;
    }
    long target = storage.nodeUnanchored(Kinds.NODE_NAMESPACE, Names.split(name));
    storage.edge(storage.fileId(), target, Kinds.EDGE_IMPORT);
    super.visit(imp, arg);
  }

  // ---- type declarations ------------------------------------------

  @Override
  public void visit(ClassOrInterfaceDeclaration coif, Void arg) {
    int nodeType = coif.isInterface() ? Kinds.NODE_INTERFACE : Kinds.NODE_CLASS;
    long typeId = emitType(coif, nodeType);
    access(coif, typeId);
    annotations(coif, typeId);
    for(ClassOrInterfaceType ext : coif.getExtendedTypes()) {
      referenceEdge(typeId, ext, Kinds.EDGE_INHERITANCE);
    }
    for(ClassOrInterfaceType impl : coif.getImplementedTypes()) {
      referenceEdge(typeId, impl, Kinds.EDGE_INHERITANCE);
    }
    for(TypeParameter tp : coif.getTypeParameters()) {
      recordTypeParameter(tp);
    }
    super.visit(coif, arg);
  }

  @Override
  public void visit(EnumDeclaration ed, Void arg) {
    long typeId = emitType(ed, Kinds.NODE_ENUM);
    access(ed, typeId);
    annotations(ed, typeId);
    for(ClassOrInterfaceType impl : ed.getImplementedTypes()) {
      referenceEdge(typeId, impl, Kinds.EDGE_INHERITANCE);
    }
    super.visit(ed, arg);
  }

  @Override
  public void visit(AnnotationDeclaration ad, Void arg) {
    long typeId = emitType(ad, Kinds.NODE_ANNOTATION);
    access(ad, typeId);
    super.visit(ad, arg);
  }

  @Override
  public void visit(RecordDeclaration rd, Void arg) {
    long typeId = emitType(rd, Kinds.NODE_CLASS);
    access(rd, typeId);
    annotations(rd, typeId);
    for(ClassOrInterfaceType impl : rd.getImplementedTypes()) {
      referenceEdge(typeId, impl, Kinds.EDGE_INHERITANCE);
    }
    for(TypeParameter tp : rd.getTypeParameters()) {
      recordTypeParameter(tp);
    }
    super.visit(rd, arg);
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
    String returnName = md.getType().asString();
    Names.Element[] chain = chainForCallable(md, returnName, md.getParameters());
    long id = storage.nodeFor(md, Kinds.NODE_METHOD, chain);
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(md, id);
    location(id, md, Kinds.LOCATION_TOKEN);
    if(md.getBody().isPresent()) {
      location(id, md, Kinds.LOCATION_SCOPE);
    }
    for(ReferenceType thrown : md.getThrownExceptions()) {
      referenceEdge(id, thrown, Kinds.EDGE_TYPE_USAGE);
    }
    annotations(md, id);
    for(TypeParameter tp : md.getTypeParameters()) {
      recordTypeParameter(tp);
    }
    super.visit(md, arg);
  }

  @Override
  public void visit(ConstructorDeclaration cd, Void arg) {
    Names.Element[] chain = chainForCallable(cd, "void", cd.getParameters());
    long id = storage.nodeFor(cd, Kinds.NODE_METHOD, chain);
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    access(cd, id);
    location(id, cd, Kinds.LOCATION_TOKEN);
    if(cd.getBody() != null) {
      location(id, cd, Kinds.LOCATION_SCOPE);
    }
    annotations(cd, id);
    super.visit(cd, arg);
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
    Names.Element[] chain;
    Optional<TypeDeclaration> parentOpt = findAncestor(var, TypeDeclaration.class);
    if(parentOpt.isPresent()) {
      chain = concat(chainForType(parentOpt.get()), Names.Element.plain(var.getNameAsString()));
    } else {
      chain = new Names.Element[]{Names.Element.plain(var.getNameAsString())};
    }
    long id = storage.nodeFor(var, Kinds.NODE_FIELD, chain);
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    if(fd.isPublic()) {
      storage.access(id, Kinds.ACCESS_PUBLIC);
    } else if(fd.isPrivate()) {
      storage.access(id, Kinds.ACCESS_PRIVATE);
    } else if(fd.isProtected()) {
      storage.access(id, Kinds.ACCESS_PROTECTED);
    }
    location(id, var, Kinds.LOCATION_TOKEN);

    com.github.javaparser.ast.type.Type ft = var.getType();
    if(ft instanceof ClassOrInterfaceType cit) {
      referenceEdge(id, cit, Kinds.EDGE_TYPE_USAGE);
    }
    annotations(fd, id);
  }

  // ---- local symbols -------------------------------------------

  @Override
  public void visit(Parameter param, Void arg) {
    recordLocal(param.getNameAsString(), param.getType());
    super.visit(param, arg);
  }

  @Override
  public void visit(TypeParameter tp, Void arg) {
    recordTypeParameter(tp);
    super.visit(tp, arg);
  }

  private void recordTypeParameter(TypeParameter tp) {
    String name = tp.getNameAsString();
    if(name == null || name.isEmpty()) {
      return;
    }
    long id = storage.localSymbol(name);
    for(ClassOrInterfaceType bound : tp.getTypeBound()) {
      referenceEdge(id, bound, Kinds.EDGE_TYPE_USAGE);
    }
  }

  private void recordLocal(String name, com.github.javaparser.ast.type.Type type) {
    if(name == null || name.isEmpty()) {
      return;
    }
    long id = storage.localSymbol(name);
    if(type instanceof ClassOrInterfaceType cit) {
      referenceEdge(id, cit, Kinds.EDGE_TYPE_USAGE);
    }
  }

  // ---- expressions -----------------------------------------------

  @Override
  public void visit(MethodCallExpr mce, Void arg) {
    String name = mce.getNameAsString();
    if(name == null || name.isEmpty()) {
      return;
    }
    long target = storage.nodeUnanchored(Kinds.NODE_METHOD, Names.Element.signature(name, "", "()"));
    storage.edge(storage.fileId(), target, Kinds.EDGE_CALL);
    super.visit(mce, arg);
  }

  @Override
  public void visit(ObjectCreationExpr oce, Void arg) {
    ClassOrInterfaceType t = oce.getType();
    String fqn = resolver.resolveType(t);
    if(fqn == null || fqn.isEmpty()) {
      return;
    }
    long target = storage.nodeUnanchored(Kinds.NODE_CLASS, Names.split(fqn));
    storage.edge(storage.fileId(), target, Kinds.EDGE_TYPE_USAGE);
    super.visit(oce, arg);
  }

  @Override
  public void visit(NameExpr ne, Void arg) {
    String name = ne.getNameAsString();
    if(name == null || name.isEmpty()) {
      return;
    }
    if(resolver.isResolvable(name)) {
      String fqn = resolver.resolveName(name);
      long target = storage.nodeUnanchored(Kinds.NODE_CLASS, Names.split(fqn));
      storage.edge(storage.fileId(), target, Kinds.EDGE_TYPE_USAGE);
    }
    super.visit(ne, arg);
  }

  // ---- helpers ---------------------------------------------------

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
    int kind = kindFor(td);
    if(!storage.known(td)) {
      storage.nodeFor(td, kind, chainForType(td));
    }
    return storage.idOf(td);
  }

  private long emitType(TypeDeclaration td, int nodeType) {
    long id = storage.nodeFor(td, nodeType, chainForType(td));
    storage.symbol(id, Kinds.DEFINITION_EXPLICIT);
    location(id, td, Kinds.LOCATION_TOKEN);
    if(td.getMembers() != null && !td.getMembers().isEmpty()) {
      location(id, td, Kinds.LOCATION_SCOPE);
    }
    return id;
  }

  private int kindFor(TypeDeclaration td) {
    if(td instanceof AnnotationDeclaration) return Kinds.NODE_ANNOTATION;
    if(td instanceof EnumDeclaration) return Kinds.NODE_ENUM;
    if(td instanceof RecordDeclaration) return Kinds.NODE_CLASS;
    if(td instanceof ClassOrInterfaceDeclaration) {
      return (((ClassOrInterfaceDeclaration) td).isInterface() ? Kinds.NODE_INTERFACE : Kinds.NODE_CLASS);
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
    }
  }

  private void annotations(NodeWithAnnotations<?> node, long nodeId) {
    for(AnnotationExpr ann : node.getAnnotations()) {
      String name = ann.getNameAsString();
      if(name == null || name.isEmpty()) {
        continue;
      }
      long target = storage.nodeUnanchored(Kinds.NODE_ANNOTATION, Names.split(name));
      storage.edge(nodeId, target, Kinds.EDGE_ANNOTATION_USAGE);
    }
  }

  private void referenceEdge(long from, ReferenceType t, int edgeType) {
    String fqn = resolver.resolveType(t);
    if(fqn == null || fqn.isEmpty()) {
      return;
    }
    long target = storage.nodeUnanchored(Kinds.NODE_CLASS, Names.split(fqn));
    storage.edge(from, target, edgeType);
  }

  private void location(long id, Node n, int type) {
    Optional<Range> range = n.getRange();
    if(range.isEmpty()) {
      return;
    }
    Range r = range.get();
    storage.location(id, r.begin.line, r.begin.column, r.end.line, r.end.column, type);
  }

  /** FQ-name element chain for a type declaration (outer → inner order). */
  private Names.Element[] chainForType(TypeDeclaration td) {
    String pkg = resolver.defaultPackage();
    String head = (pkg == null || pkg.isEmpty()) ? "" : pkg;
    List<String> outs = new ArrayList<>();
    Node cur = td.getParentNode().orElse(null);
    while(cur != null) {
      if(cur instanceof TypeDeclaration t) {
        outs.add(t.getNameAsString());
      }
      cur = cur.getParentNode().orElse(null);
    }
    StringBuilder sb = new StringBuilder(head);
    if(sb.length() > 0) {
      sb.append('.');
    }
    for(int i = outs.size() - 1; i >= 0; i--) {
      sb.append(outs.get(i)).append('.');
    }
    sb.append(td.getNameAsString());
    return Names.split(sb.toString());
  }

  /** Method name chain with return type + parameters forming a disambiguating signature. */
  private Names.Element[] chainForCallable(CallableDeclaration<?> md, String returnName,
                                            com.github.javaparser.ast.NodeList<Parameter> params) {
    StringBuilder joined = new StringBuilder();
    for(int i = 0; i < params.size(); i++) {
      if(i > 0) {
        joined.append(", ");
      }
      Parameter p = params.get(i);
      String pname = p.getType().asString();
      if(p.isVarArgs()) {
        pname = pname + "[]";
      }
      joined.append(pname);
    }
    String sigPost = "(" + joined + ")";

    Names.Element[] parent = new Names.Element[0];
    Optional<TypeDeclaration> parentOpt = findAncestor(md, TypeDeclaration.class);
    if(parentOpt.isPresent()) {
      parent = chainForType(parentOpt.get());
    }
    Names.Element[] out = new Names.Element[parent.length + 1];
    System.arraycopy(parent, 0, out, 0, parent.length);
    out[parent.length] = Names.Element.signature(md.getNameAsString(), returnName, sigPost);
    return out;
  }

  private static Names.Element[] concat(Names.Element[] base, Names.Element extra) {
    Names.Element[] out = new Names.Element[base.length + 1];
    System.arraycopy(base, 0, out, 0, base.length);
    out[base.length] = extra;
    return out;
  }
}
