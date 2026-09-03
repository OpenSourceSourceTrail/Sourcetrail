#include "data/parser/cxx/ASTConsumer.h"
// internal
#include "data/parser/cxx/CxxAstVisitor.h"
#include "data/parser/cxx/CxxVerboseAstVisitor.h"
#include "Profiling.h"
#include "settings/IApplicationSettings.hpp"

ASTConsumer::ASTConsumer(clang::ASTContext* context,
                         clang::Preprocessor* preprocessor,
                         std::shared_ptr<ParserClient> client,
                         std::shared_ptr<CanonicalFilePathCache> canonicalFilePathCache,
                         std::shared_ptr<IndexerStateInfo> indexerStateInfo) {
  auto* pAppSettings = IApplicationSettings::getInstanceRaw();

  if(pAppSettings->getLoggingEnabled() && pAppSettings->getVerboseIndexerLoggingEnabled()) {
    m_visitor = std::make_shared<CxxVerboseAstVisitor>(context, preprocessor, client, canonicalFilePathCache, indexerStateInfo);
  } else {
    m_visitor = std::make_shared<CxxAstVisitor>(context, preprocessor, client, canonicalFilePathCache, indexerStateInfo);
  }
}

ASTConsumer::~ASTConsumer() = default;

void ASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  SR_ZONE_N("cxx/indexDecl");
  m_visitor->indexDecl(context.getTranslationUnitDecl());
}