#pragma once
#include <atomic>

#include <QObject>
#include <QString>

#include "Controller.h"
#include "LlmCoordinator.hpp"

class ChatView;
class ChatModel;
namespace sourcetrail::lib_llm::services {
class LlmCoordinator;
}

class ChatController
    : public QObject
    , public Controller {
  Q_OBJECT
public:
  explicit ChatController(std::shared_ptr<ChatModel> model,
                          std::shared_ptr<sourcetrail::lib_llm::services::LlmCoordinator> llmService,
                          QObject* parent = nullptr) noexcept;
  Q_DISABLE_COPY_MOVE(ChatController)
  ~ChatController() noexcept override;

  void attachView(ChatView* view);
  void clear() override {}

public slots:
  void handleUserMessage(const QString& content);
  void handleClearRequest();

signals:
  void processingStarted();
  void processingCompleted();
  void errorOccurred(const QString& error);

private slots:
  void onLLMResponseReceived(const QString& response);
  void onLLMError(const QString& error);

private:
  std::shared_ptr<ChatModel> mModel;
  std::shared_ptr<sourcetrail::lib_llm::services::LlmCoordinator> mCoordinator;
  QFuture<nonstd::expected<sourcetrail::lib_llm::Message, sourcetrail::lib_llm::LlmError>> mCurrentRequest;
  ChatView* mView{nullptr};    // Non-owning
  std::atomic_bool mIsProcessing{false};
};
