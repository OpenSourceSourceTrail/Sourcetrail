#pragma once
#include <memory>

#include <QString>

#include <nonstd/expected.hpp>

#include "LlmTypes.hpp"

class StorageAccess;

namespace sourcetrail::lib_llm::services {

struct ContextSegment final {
  QString label;           // e.g., "Function Definition", "Call Graph"
  QString content;         // The actual code or relationship data
  AttachmentRef source;    // Traceability back to source
  size_t estimatedTokens = 0;
};

struct LlmRequest final {
  QString userPrompt;
  std::vector<ContextSegment> contextSegments;
  ConversationHistory history;
  size_t estimatedTotalTokens = 0;
};

struct ContextConfig final {
  size_t maxTokens = 180'000;
  size_t maxReferenceDepth = 2;
  size_t maxFileLines = 500;
  bool includeCallers = true;
  bool includeCallees = true;
  bool includeReferences = true;
};

class LlmContextBuilder {
public:
  explicit LlmContextBuilder(std::shared_ptr<StorageAccess> storageAccess,
                             std::shared_ptr<ContextConfig> config = std::make_shared<ContextConfig>());

  [[nodiscard]] nonstd::expected<LlmRequest, LlmError> buildContext(const QString& userPrompt,
                                                                    const std::vector<AttachmentRef>& attachments,
                                                                    const ConversationHistory& history) const;

private:
  [[nodiscard]] nonstd::expected<ContextSegment, LlmError> buildSymbolContext(const AttachmentRef& ref) const;

  [[nodiscard]] nonstd::expected<ContextSegment, LlmError> buildFileContext(const AttachmentRef& ref) const;

  [[nodiscard]] std::vector<ContextSegment> gatherRelatedSymbols(SymbolId id, size_t maxDepth) const;

  [[nodiscard]] size_t estimateTokens(const QString& text) const;

  void truncateToFit(LlmRequest& request, size_t maxTokens) const;

  std::shared_ptr<StorageAccess> m_storageAccess;
  std::shared_ptr<ContextConfig> m_config;
};

}    // namespace sourcetrail::lib_llm::services
