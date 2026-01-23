#include "ChatController.hpp"

#include "ChatMessage.hpp"
#include "ChatModel.hpp"
#include "ChatView.hpp"
#include "ILLMService.hpp"


ChatController::ChatController(std::shared_ptr<ChatModel> model, std::shared_ptr<ILLMService> llmService, QObject* parent) noexcept
    : QObject{parent}, mModel{std::move(model)}, mLlmService{std::move(llmService)} {}

ChatController::~ChatController() noexcept = default;

void ChatController::attachView(ChatView* view) {
  mView = view;

  if(nullptr != mView) {
    connect(mView, &ChatView::messageSubmitted, this, &ChatController::handleUserMessage);
    connect(mView, &ChatView::clearRequested, this, &ChatController::handleClearRequest);
  }
}

void ChatController::handleUserMessage(const QString& content) {
  if(mIsProcessing) {
    return;    // Prevent concurrent requests
  }

  // Add user message to model
  auto result = mModel->addMessage(ChatMessage{content, MessageRole::User});

  if(!result) {
    if(nullptr != mView) {
      // View updates automatically via model signals
    }
    return;
  }

  // Clear input after successful submission
  if(nullptr != mView) {
    mView->clearInput();
    mView->setInputEnabled(false);
  }

  mIsProcessing = true;
  emit processingStarted();

  // Send to LLM service (async)
  if(mLlmService && mLlmService->isAvailable()) {
    mLlmService->sendMessage(content);
  } else {
    onLLMError("LLM service unavailable");
  }
}

void ChatController::handleClearRequest() {
  if(mIsProcessing) {
    if(mLlmService) {
      mLlmService->cancelRequest();
    }
    mIsProcessing = false;
  }

  mModel->clear();

  if(nullptr != mView) {
    mView->setInputEnabled(true);
    mView->focusInput();
  }
}

void ChatController::onLLMResponseReceived(const QString& response) {
  mIsProcessing = false;

  auto result = mModel->addMessage(ChatMessage{response, MessageRole::Assistant});

  if(nullptr != mView) {
    mView->setInputEnabled(true);
    mView->focusInput();
  }

  emit processingCompleted();
}

void ChatController::onLLMError(const QString& error) {
  mIsProcessing = false;

  mModel->addMessage(ChatMessage{QString{"Error: %1"}.arg(error), MessageRole::Error});

  if(nullptr != mView) {
    mView->setInputEnabled(true);
  }

  emit errorOccurred(error);
}
