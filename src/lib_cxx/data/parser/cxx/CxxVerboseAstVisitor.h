#pragma once
#include <clang/AST/TypeLoc.h>

#include "CxxAstVisitor.h"

class CanonicalFilePathCache;
class ParserClient;

class CxxVerboseAstVisitor : public CxxAstVisitor {
public:
  CxxVerboseAstVisitor(clang::ASTContext* context,
                       clang::Preprocessor* preprocessor,
                       const std::shared_ptr<ParserClient>& client,
                       const std::shared_ptr<CanonicalFilePathCache>& canonicalFilePathCache,
                       const std::shared_ptr<IndexerStateInfo>& indexerStateInfo);

private:
  using base = CxxAstVisitor;

  bool TraverseDecl(clang::Decl* decl) override;
  bool TraverseStmt(clang::Stmt* stmt) override;
  bool TraverseTypeLoc(clang::TypeLoc type) override;

  std::string getIndentString() const;

  FilePath mCurrentFilePath;
  std::uint32_t mIndentation = 0;
};
