#include "CxxVerboseAstVisitor.h"

#include <sstream>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#include "CanonicalFilePathCache.h"
#include "logging.h"
#include "ParseLocation.h"
#include "ParserClient.h"
#include "ScopedSwitcher.h"

namespace {

std::string typeLocClassToString(clang::TypeLoc typeLoc) {
  switch(typeLoc.getTypeLocClass()) {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STRINGIFY(X) #X
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ABSTRACT_TYPE(Class, Base)
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPE(Class, Base)                                                                                                        \
  case clang::TypeLoc::Class:                                                                                                    \
    return STRINGIFY(Class);
#include <clang/AST/TypeLoc.h>
  case clang::TypeLoc::TypeLocClass::Qualified:
    return "Qualified";
  default:
    return "";
  }
}

std::string obfuscateName(const std::string& name) {
  if(name.length() <= 2) {
    return name;
  }
  return name.substr(0, 1) + ".." + name.substr(name.length() - 1);
}

}    // namespace

CxxVerboseAstVisitor::CxxVerboseAstVisitor(clang::ASTContext* context,
                                           clang::Preprocessor* preprocessor,
                                           const std::shared_ptr<ParserClient>& client,
                                           const std::shared_ptr<CanonicalFilePathCache>& canonicalFilePathCache,
                                           const std::shared_ptr<IndexerStateInfo>& indexerStateInfo)
    : base(context, preprocessor, client, canonicalFilePathCache, indexerStateInfo) {}

bool CxxVerboseAstVisitor::TraverseDecl(clang::Decl* decl) {
  if(nullptr != decl) {
    std::stringstream stream;
    stream << getIndentString() << decl->getDeclKindName() << "Decl";
    if(auto* namedDecl = clang::dyn_cast_or_null<clang::NamedDecl>(decl)) {
      stream << " [" << obfuscateName(namedDecl->getNameAsString()) << "]";
    }

    const ParseLocation loc = getParseLocation(decl->getSourceRange());
    stream << " <" << loc.startLineNumber << ":" << loc.startColumnNumber << ", " << loc.endLineNumber << ":"
           << loc.endColumnNumber << ">";

    const clang::SourceManager& sourceManager = m_astContext->getSourceManager();
    const FilePath currentFilePath = getCanonicalFilePathCache()->getCanonicalFilePath(
        sourceManager.getFileID(decl->getSourceRange().getBegin()), sourceManager);
    if(mCurrentFilePath != currentFilePath) {
      mCurrentFilePath = currentFilePath;
      LOG_INFO(L"Indexer - Traversing \"{}\"", currentFilePath.wstr());
    }

    LOG_INFO("Indexer - {}", stream.str());

    {
      const ScopedSwitcher<unsigned int> switcher(mIndentation, mIndentation + 1);
      return base::TraverseDecl(decl);
    }
  }
  return true;
}

bool CxxVerboseAstVisitor::TraverseStmt(clang::Stmt* stmt) {
  if(nullptr != stmt) {
    const ParseLocation loc = getParseLocation(stmt->getSourceRange());
    LOG_INFO(fmt::format("Indexer - {}{} <{}:{}, {}:{}>",
                         getIndentString(),
                         stmt->getStmtClassName(),
                         loc.startLineNumber,
                         loc.startColumnNumber,
                         loc.endLineNumber,
                         loc.endColumnNumber));
    {
      const ScopedSwitcher<unsigned int> switcher(mIndentation, mIndentation + 1);
      return base::TraverseStmt(stmt);
    }
  }
  return true;
}

bool CxxVerboseAstVisitor::TraverseTypeLoc(clang::TypeLoc type) {
  if(!type.isNull()) {
    const ParseLocation loc = getParseLocation(type.getSourceRange());
    LOG_INFO(fmt::format(fmt::runtime("Indexer - {}{}TypeLoc <{}:{}, {}:{}>"),
                         getIndentString(),
                         typeLocClassToString(type),
                         loc.startColumnNumber,
                         loc.endLineNumber,
                         loc.endColumnNumber));
    {
      const ScopedSwitcher<unsigned int> switcher(mIndentation, mIndentation + 1);
      return base::TraverseTypeLoc(type);
    }
  }
  return true;
}

std::string CxxVerboseAstVisitor::getIndentString() const {
  std::stringstream indentString;
  for(unsigned int i = 0; i < mIndentation; i++) {
    indentString << "| ";
  }
  return indentString.str();
}
