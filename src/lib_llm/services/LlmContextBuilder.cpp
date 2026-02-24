#include "LlmContextBuilder.hpp"

#include <algorithm>
#include <optional>

#include <QtConcurrent>

#include <nonstd/expected.hpp>

#include "StorageAccess.h"    // From main codebase

namespace sourcetrail::lib_llm::services {

LlmContextBuilder::LlmContextBuilder(std::shared_ptr<StorageAccess> storageAccess, std::shared_ptr<ContextConfig> config)
    : m_storageAccess(std::move(storageAccess)), m_config(std::move(config)) {}

nonstd::expected<LlmRequest, LlmError> LlmContextBuilder::buildContext(const QString& userPrompt,
                                                                       const std::vector<AttachmentRef>& attachments,
                                                                       const ConversationHistory& history) const {
  LlmRequest request;
  request.userPrompt = userPrompt;
  request.history = history;

  // Build context segments in parallel for performance
  QList<QFuture<nonstd::expected<ContextSegment, LlmError>>> futures;

  for(const auto& attachment : attachments) {
    if(!attachment.isValid()) {
      return nonstd::unexpected<LlmError>(LlmError{
          .type = LlmError::Type::InvalidInput, .message = "Invalid attachment reference", .retryAfterSeconds = std::nullopt});
    }

    futures.append(QtConcurrent::run([this, attachment]() {
      if(attachment.type == AttachmentRef::Type::Symbol || attachment.type == AttachmentRef::Type::GraphNode) {
        return buildSymbolContext(attachment);
      } else {
        return buildFileContext(attachment);
      }
    }));
  }

  // Wait for all context building tasks
  for(auto& future : futures) {
    auto result = future.result();
    if(!result) {
      return nonstd::unexpected<LlmError>(result.error());
    }
    request.contextSegments.push_back(std::move(*result));
  }

  // Estimate total tokens
  request.estimatedTotalTokens = estimateTokens(userPrompt);
  for(const auto& segment : request.contextSegments) {
    request.estimatedTotalTokens += segment.estimatedTokens;
  }
  for(const auto& msg : history.messages) {
    request.estimatedTotalTokens += estimateTokens(msg.content);
  }

  // Truncate if needed
  if(request.estimatedTotalTokens > m_config->maxTokens) {
    truncateToFit(request, m_config->maxTokens);
  }

  return request;
}

nonstd::expected<ContextSegment, LlmError> LlmContextBuilder::buildSymbolContext(const AttachmentRef& ref) const {
  if(!ref.symbolId) {
    return nonstd::unexpected<LlmError>(LlmError{.type = LlmError::Type::ContextBuildFailure,
                                                 .message = "Symbol attachment missing symbolId",
                                                 .retryAfterSeconds = std::nullopt});
  }

  // Fetch symbol data from storage
  // Note: This assumes StorageAccess has these methods
  // Actual implementation depends on Sourcetrail's storage layer

  ContextSegment segment;
  segment.source = ref;
  segment.label = "Symbol Definition";

  // Example: Build symbol context
  // In real implementation, query the storage layer
  segment.content = QString("// Symbol ID: %1\n").arg(*ref.symbolId);
  segment.content += "// TODO: Fetch actual symbol definition from StorageAccess\n";

  // Gather related symbols (callers, callees, references)
  auto relatedSegments = gatherRelatedSymbols(*ref.symbolId, m_config->maxReferenceDepth);
  for(const auto& related : relatedSegments) {
    segment.content += "\n" + related.content;
  }

  segment.estimatedTokens = estimateTokens(segment.content);

  return segment;
}

nonstd::expected<ContextSegment, LlmError> LlmContextBuilder::buildFileContext(const AttachmentRef& ref) const {
  if(!ref.filePath) {
    return nonstd::unexpected<LlmError>(
        LlmError{.type = LlmError::Type::ContextBuildFailure, .message = "File attachment missing filePath"});
  }

  ContextSegment segment;
  segment.source = ref;
  segment.label = QString("File: %1").arg(*ref.filePath);

  // Read file content (with range restriction if provided)
  QFile file(*ref.filePath);
  if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return nonstd::unexpected<LlmError>(
        LlmError{.type = LlmError::Type::ContextBuildFailure, .message = QString("Failed to open file: %1").arg(*ref.filePath)});
  }

  QTextStream stream(&file);
  QString content;
  int lineNum = 0;

  while(!stream.atEnd()) {
    QString line = stream.readLine();
    lineNum++;

    if(ref.range) {
      if(lineNum >= ref.range->startLine && lineNum <= ref.range->endLine) {
        content += line + "\n";
      }
    } else {
      content += line + "\n";
      if(lineNum >= static_cast<int>(m_config->maxFileLines)) {
        content += QString("// ... (truncated at %1 lines)\n").arg(m_config->maxFileLines);
        break;
      }
    }
  }

  segment.content = content;
  segment.estimatedTokens = estimateTokens(content);

  return segment;
}

std::vector<ContextSegment> LlmContextBuilder::gatherRelatedSymbols(SymbolId id, size_t maxDepth) const {
  std::vector<ContextSegment> segments;

  if(maxDepth == 0) {
    return segments;
  }

  // TODO: Implement actual reference traversal using StorageAccess
  // This would query:
  // - Functions that call this function (callers)
  // - Functions this function calls (callees)
  // - Variables referenced
  // - Types used

  return segments;
}

size_t LlmContextBuilder::estimateTokens(const QString& text) const {
  // Rough estimation: ~4 characters per token for code
  // More sophisticated tokenization would use tiktoken or similar
  return text.length() / 4;
}

void LlmContextBuilder::truncateToFit(LlmRequest& request, size_t maxTokens) const {
  // Keep user prompt and recent history, truncate context segments
  size_t baseTokens = estimateTokens(request.userPrompt);

  // Reserve tokens for recent messages (keep last 5)
  size_t historyTokens = 0;
  int messagesToKeep = std::min(5, static_cast<int>(request.history.messages.size()));
  for(int i = request.history.messages.size() - messagesToKeep; i < static_cast<int>(request.history.messages.size()); ++i) {
    historyTokens += estimateTokens(request.history.messages[i].content);
  }

  size_t availableForContext = maxTokens - baseTokens - historyTokens;

  // Truncate context segments proportionally
  size_t currentContextTokens = 0;
  for(const auto& segment : request.contextSegments) {
    currentContextTokens += segment.estimatedTokens;
  }

  if(currentContextTokens > availableForContext) {
    double scaleFactor = static_cast<double>(availableForContext) / currentContextTokens;
    for(auto& segment : request.contextSegments) {
      size_t targetTokens = segment.estimatedTokens * scaleFactor;
      size_t targetChars = targetTokens * 4;
      if(segment.content.length() > static_cast<int>(targetChars)) {
        segment.content = segment.content.left(targetChars) + "\n// ... (truncated)";
        segment.estimatedTokens = targetTokens;
      }
    }
  }

  request.estimatedTotalTokens = baseTokens + historyTokens + availableForContext;
}

}    // namespace sourcetrail::lib_llm::services
