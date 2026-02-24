#pragma once
#include <cstdint>
#include <optional>

#include <QDateTime>
#include <QString>

namespace sourcetrail::lib_llm {
// Type aliases for clarity
using ConversationId = int64_t;
using MessageId = int64_t;
using SymbolId = int64_t;
using ProjectId = int64_t;

struct TextRange final {
  int startLine;
  int startColumn;
  int endLine;
  int endColumn;
};

struct AttachmentRef final {
  enum class Type : uint8_t {
    Symbol,        // Function, class, variable, etc.
    File,          // Entire source file
    GraphNode,     // Visual node from graph view
    CodeSnippet    // User-selected text range
  };

  Type type{};
  std::optional<SymbolId> symbolId;
  std::optional<QString> filePath;
  std::optional<TextRange> range;

  [[nodiscard]] bool isValid() const {
    switch(type) {
    case Type::Symbol:
    case Type::GraphNode:
      return symbolId.has_value();
    case Type::File:
      return filePath.has_value();
    case Type::CodeSnippet:
      return filePath.has_value() && range.has_value();
    }
    return false;
  }
};

struct LlmError final {
  enum class Type : uint8_t { ContextBuildFailure, NetworkError, ApiError, StorageError, Cancelled, InvalidInput, RateLimited };

  Type type;
  QString message;
  std::optional<int> retryAfterSeconds;
};

struct Message final {
  MessageId id;
  QString role;    // "user" or "assistant"
  QString content;
  QDateTime createdAt;
  std::optional<size_t> tokensUsed;
  std::vector<AttachmentRef> attachments;
};

struct UserMessage final {
  QString prompt;
  std::vector<AttachmentRef> attachments;
  ConversationId conversationId;
};

struct Conversation final {
  ConversationId id;
  ProjectId projectId;
  QString title;
  QDateTime createdAt;
  QDateTime updatedAt;
};

struct ConversationHistory final {
  std::vector<Message> messages;
  size_t totalTokens = 0;
};
}    // namespace sourcetrail::lib_llm
