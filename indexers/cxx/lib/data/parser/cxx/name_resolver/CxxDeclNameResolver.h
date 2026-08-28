#ifndef CXX_DECL_NAME_RESOLVER_H
#define CXX_DECL_NAME_RESOLVER_H

#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>

#include "data/parser/cxx/name/CxxDeclName.h"
#include "data/parser/cxx/name/CxxTypeName.h"
#include "data/parser/cxx/name_resolver/CxxNameResolver.h"
#include "data/parser/cxx/name_resolver/CxxTypeNameResolver.h"
#include "utilityString.h"

class CanonicalFilePathCache;

class CxxDeclNameResolver : public CxxNameResolver {
public:
  CxxDeclNameResolver(CanonicalFilePathCache* canonicalFilePathCache);
  CxxDeclNameResolver(const CxxNameResolver* other);

  std::unique_ptr<CxxDeclName> getName(const clang::NamedDecl* declaration);

private:
  std::unique_ptr<CxxName> getContextName(const clang::DeclContext* declaration);
  std::unique_ptr<CxxDeclName> getDeclName(const clang::NamedDecl* declaration);
  std::wstring getTranslationUnitMainFileName(const clang::Decl* declaration);
  std::wstring getNameForAnonymousSymbol(const std::wstring& symbolKindName, const clang::Decl* declaration);
  std::vector<std::wstring> getTemplateParameterStrings(const clang::TemplateDecl* templateDecl);
  template <typename T>
  std::vector<std::wstring> getTemplateParameterStringsOfPartialSpecialization(const T* templateDecl);
  std::wstring getTemplateParameterString(const clang::NamedDecl* parameter);
  std::wstring getTemplateArgumentName(const clang::TemplateArgument& argument);

  const clang::NamedDecl* m_currentDecl;
};


template <typename T>
std::vector<std::wstring> CxxDeclNameResolver::getTemplateParameterStringsOfPartialSpecialization(const T* partialSpecializationDecl) {
  std::vector<std::wstring> templateParameterNames;
  clang::TemplateParameterList* parameterList = partialSpecializationDecl->getTemplateParameters();

  const clang::TemplateArgumentList& templateArgumentList = partialSpecializationDecl->getTemplateArgs();
  for(unsigned i = 0; i < templateArgumentList.size(); i++) {
    const clang::TemplateArgument& templateArgument = templateArgumentList.get(i);
    const clang::TemplateArgument::ArgKind argKind = templateArgument.getKind();
    if(templateArgument.isDependent()) {
      if(argKind == clang::TemplateArgument::Type && !templateArgument.getAsType().isNull()) {
        const clang::Type* argumentType = templateArgument.getAsType().getTypePtr();
        if(const clang::TemplateTypeParmType* ttpt = clang::dyn_cast<clang::TemplateTypeParmType>(argumentType)) {
          if(ttpt->getDepth() == parameterList->getDepth()) {
            templateParameterNames.push_back(getTemplateParameterString(parameterList->getParam(ttpt->getIndex())));
          } else {
            // TODO: fix case when arg depends on template parameter of outer template
            // class, or depends on first template parameter.
            templateParameterNames.push_back(L"arg" + std::to_wstring(ttpt->getDepth()) + L"_" + std::to_wstring(ttpt->getIndex()));
          }
        } else {
          templateParameterNames.push_back(
              CxxTypeName::makeUnsolvedIfNull(CxxTypeNameResolver(this).getName(argumentType))->toString());
        }
      } else if(argKind == clang::TemplateArgument::Template && !templateArgument.getAsTemplate().isNull()) {
        const clang::TemplateTemplateParmDecl* decl = clang::dyn_cast<clang::TemplateTemplateParmDecl>(
            templateArgument.getAsTemplate().getAsTemplateDecl());
        if(decl) {
          if(decl->getDepth() == parameterList->getDepth()) {
            templateParameterNames.push_back(getTemplateParameterString(parameterList->getParam(decl->getIndex())));
          } else {
            // TODO: fix case when arg depends on template parameter of outer template
            // class, or depends on first template parameter.
            templateParameterNames.push_back(L"arg" + std::to_wstring(decl->getDepth()) + L"_" + std::to_wstring(decl->getIndex()));
          }
        } else {
          templateParameterNames.push_back(getTemplateArgumentName(templateArgument));
        }
      } else if(argKind == clang::TemplateArgument::Expression) {
        // A non-type template parameter used as an argument, e.g. the `q` of `class A<&g_p, q>`.
        // Clang 21 canonicalizes these, so printing the argument yields "value-parameter-0-0"
        // rather than the written name; recover it from the parameter list as above.
        const clang::NonTypeTemplateParmDecl* decl = nullptr;
        if(const clang::Expr* expr = templateArgument.getAsExpr()) {
          if(const auto* declRef = clang::dyn_cast<clang::DeclRefExpr>(expr->IgnoreParenImpCasts())) {
            decl = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(declRef->getDecl());
          }
        }

        // Only the written name, not the full parameter string the type/template branches use above:
        // `class A<&g_p, q>` names its second argument `q`, not `P * q`.
        if(decl != nullptr && decl->getDepth() == parameterList->getDepth() && !decl->getName().empty()) {
          templateParameterNames.push_back(utility::decodeFromUtf8(parameterList->getParam(decl->getIndex())->getName().str()));
        } else if(decl != nullptr && decl->getDepth() != parameterList->getDepth()) {
          // TODO: fix case when arg depends on template parameter of outer template
          // class, or depends on first template parameter.
          templateParameterNames.push_back(L"arg" + std::to_wstring(decl->getDepth()) + L"_" + std::to_wstring(decl->getIndex()));
        } else {
          templateParameterNames.push_back(getTemplateArgumentName(templateArgument));
        }
      } else {
        templateParameterNames.push_back(getTemplateArgumentName(templateArgument));
      }
    } else {
      templateParameterNames.push_back(getTemplateArgumentName(templateArgument));
    }
  }
  return templateParameterNames;
}

#endif    // CXX_DECL_NAME_RESOLVER_H
