#include "CxxSpecifierNameResolver.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/PrettyPrinter.h>

#include "CxxDeclNameResolver.h"
#include "CxxTypeNameResolver.h"
#include "utilityString.h"

CxxSpecifierNameResolver::CxxSpecifierNameResolver(CanonicalFilePathCache* canonicalFilePathCache)
    : CxxNameResolver(canonicalFilePathCache) {}

CxxSpecifierNameResolver::CxxSpecifierNameResolver(const CxxNameResolver* other) : CxxNameResolver(other) {}

std::unique_ptr<CxxName> CxxSpecifierNameResolver::getName(clang::NestedNameSpecifier nestedNameSpecifier) {
  switch(nestedNameSpecifier.getKind()) {
  case clang::NestedNameSpecifier::Kind::Namespace:
    // covers both namespaces and namespace aliases: clang::NamespaceAliasDecl derives from clang::NamespaceBaseDecl.
    // The parent hierarchy comes from the decl context, so the prefix does not have to be walked here.
    return CxxDeclNameResolver(this).getName(nestedNameSpecifier.getAsNamespaceAndPrefix().Namespace);

  case clang::NestedNameSpecifier::Kind::Type:
    // a dependent qualifier such as "typename T::type" is a clang::DependentNameType and lands here as well
    return CxxTypeName::makeUnsolvedIfNull(CxxTypeNameResolver(this).getName(nestedNameSpecifier.getAsType()));

  case clang::NestedNameSpecifier::Kind::MicrosoftSuper:
    return CxxDeclNameResolver(this).getName(nestedNameSpecifier.getAsRecordDecl());

  case clang::NestedNameSpecifier::Kind::Global:
  case clang::NestedNameSpecifier::Kind::Null:
    // no context name hierarchy needed.
    break;
  }

  return nullptr;
}
