#include "ChatController.hpp"

#include <utility>

#include <QTimer>

#include "ChatView.hpp"

ChatController::ChatController() noexcept = default;

ChatController::~ChatController() noexcept = default;

void ChatController::sendMessage(const QString& message) {
  getView<ChatView>()->addMessage(message, Role::User);
  QTimer::singleShot(1000, this, [this, message]() {
    // Simulate response from LLM
    onResponseReceived("This is a simulated response to your message: " + message);
  });
  // mLlmInterface->sendPrompt(message);
}

void ChatController::onResponseReceived(const QString& response) {
  getView<ChatView>()->addMessage(response, Role::Assistant);
}
