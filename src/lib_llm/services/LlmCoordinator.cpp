#include "LlmCoordinator.hpp"

#include <optional>

#include <QtConcurrent>

namespace sourcetrail::lib_llm::services {

LlmCoordinator::LlmCoordinator(QObject* parent) noexcept : QObject{parent} {}

LlmCoordinator::~LlmCoordinator() noexcept = default;

nonstd::expected<void, std::string> LlmCoordinator::registerProvider(std::shared_ptr<ILLMService> service) noexcept {
  const QString providerName = service->providerName();
  if(providers_.contains(providerName)) {
    return nonstd::make_unexpected<std::string>("Provider already registered: " + providerName.toStdString());
  }

  providers_.emplace(providerName, std::move(service));
  if(providers_.size() == 1) {
    activeProviderIt_ = providers_.begin();
  }

  emit providerRegistered(providerName);
  return {};
}

QFuture<nonstd::expected<Message, LlmError>> LlmCoordinator::sendUserMessage(const UserMessage& userMessage) noexcept {
  if(providers_.isEmpty()) {
    return QtConcurrent::run([]() -> nonstd::expected<Message, LlmError> {
      return nonstd::unexpected<LlmError>(
          LlmError{.type = LlmError::Type::InvalidInput, .message = "", .retryAfterSeconds = std::nullopt});
    });
  }

  const auto activeProvider = activeProviderIt_.value();
  return QtConcurrent::run([activeProvider, userMessage]() -> nonstd::expected<Message, LlmError> {
           return activeProvider->sendMessage(userMessage);
         })
      .then([this](nonstd::expected<Message, LlmError> result) {
        if(result) {
          emit responseReceived(*result);
        }
        return result;
      })
      .onCanceled([]() -> nonstd::expected<Message, LlmError> {
        return nonstd::unexpected<LlmError>(
            LlmError{.type = LlmError::Type::Cancelled, .message = "Request was cancelled", .retryAfterSeconds = std::nullopt});
      })
      .onFailed([](std::exception_ptr) -> nonstd::expected<Message, LlmError> {
        return nonstd::unexpected<LlmError>(LlmError{
            .type = LlmError::Type::NetworkError, .message = "An unexpected error occurred", .retryAfterSeconds = std::nullopt});
      });
}
}    // namespace sourcetrail::lib_llm::services
