#pragma once
#include <QString>

#include <nonstd/expected.hpp>

#include "LlmTypes.hpp"

namespace sourcetrail::lib_llm::services {
/**
 * @brief Interface for LLM service abstraction
 *
 * Allows dependency injection and testing without actual LLM calls.
 */
class ILLMService {
public:
  virtual ~ILLMService() noexcept;

  [[nodiscard]] virtual QString providerName() const noexcept = 0;
  virtual nonstd::expected<Message, LlmError> sendMessage(const UserMessage& message) = 0;
  virtual nonstd::expected<void, LlmError> cancelRequest() = 0;
  [[nodiscard]] virtual bool isAvailable() const noexcept = 0;
};
}    // namespace sourcetrail::lib_llm::services
