#pragma once
#include <memory>

#include <QFuture>
#include <QHash>
#include <QObject>

#include <nonstd/expected.hpp>

#include "ILLMService.hpp"
#include "LlmTypes.hpp"

namespace sourcetrail::lib_llm::services {

class LlmCoordinator : public QObject {
  Q_OBJECT

public:
  explicit LlmCoordinator(QObject* parent = nullptr) noexcept;
  Q_DISABLE_COPY_MOVE(LlmCoordinator)
  ~LlmCoordinator() noexcept override;

signals:
  void providerRegistered(const QString& providerName);
  void responseReceived(const Message& message);

public:
  nonstd::expected<void, std::string> registerProvider(std::shared_ptr<ILLMService> service) noexcept;

  QFuture<nonstd::expected<Message, LlmError>> sendUserMessage(const UserMessage& userMessage) noexcept;

private:
  QHash<QString, std::shared_ptr<ILLMService>> providers_;
  decltype(providers_)::iterator activeProviderIt_;
};

}    // namespace sourcetrail::lib_llm::services
