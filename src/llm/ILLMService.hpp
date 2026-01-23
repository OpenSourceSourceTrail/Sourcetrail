#pragma once
#include <QString>

/**
 * @brief Interface for LLM service abstraction
 *
 * Allows dependency injection and testing without actual LLM calls.
 */
class ILLMService {
public:
  virtual ~ILLMService() noexcept;

  virtual void sendMessage(const QString& message) = 0;
  virtual void cancelRequest() = 0;
  [[nodiscard]] virtual bool isAvailable() const noexcept = 0;
};
