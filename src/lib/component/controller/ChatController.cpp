#include "ChatController.hpp"

#include "ChatMessage.hpp"
#include "ChatModel.hpp"
#include "ChatView.hpp"
#include "LlmCoordinator.hpp"
#include "LlmTypes.hpp"
#include "logging.h"


ChatController::ChatController(std::shared_ptr<ChatModel> model,
                               std::shared_ptr<sourcetrail::lib_llm::services::LlmCoordinator> coordinator,
                               QObject* parent) noexcept
    : QObject{parent}, mModel{std::move(model)}, mCoordinator{std::move(coordinator)} {}

ChatController::~ChatController() noexcept = default;

void ChatController::attachView(ChatView* view) {
  mView = view;

  if(nullptr != mView) {
    connect(mView, &ChatView::messageSubmitted, this, &ChatController::handleUserMessage);
    connect(mView, &ChatView::clearRequested, this, &ChatController::handleClearRequest);
  }
}

void ChatController::handleUserMessage(const QString& content) {
  if(mIsProcessing.exchange(true)) {
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
  if(mCoordinator) {
    mCoordinator
        ->sendUserMessage(sourcetrail::lib_llm::UserMessage{
            .prompt = content,
        })
        .then([this](nonstd::expected<sourcetrail::lib_llm::Message, sourcetrail::lib_llm::LlmError> response) {
          if(response) {
            onLLMResponseReceived(response->content);
          } else {
            onLLMError(response.error().message);
          }
        });
  } else {
    onLLMError("LLM service unavailable");
  }
}

void ChatController::handleClearRequest() {
  if(mIsProcessing) {
    if(mCoordinator && mCurrentRequest.isRunning()) {
      mCurrentRequest.cancel();
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

  if(auto result = mModel->addMessage(ChatMessage{QString{"Error: %1"}.arg(error), MessageRole::Error}); !result) {
    LOG_ERROR("Failed to add error message to model: {}", error.toStdString());
  }

  if(nullptr != mView) {
    mView->setInputEnabled(true);
  }

  emit errorOccurred(error);
}
