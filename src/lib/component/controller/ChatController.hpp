#pragma once
#include <QObject>
#include <QString>

#include "Controller.h"

class ChatView;
class ChatModel;
class ILLMService;

class ChatController
    : public QObject
    , public Controller {
  Q_OBJECT
public:
  explicit ChatController(std::shared_ptr<ChatModel> model,
                          std::shared_ptr<ILLMService> llmService,
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
  std::shared_ptr<ILLMService> mLlmService;
  ChatView* mView{nullptr};    // Non-owning
  bool mIsProcessing{false};
};
