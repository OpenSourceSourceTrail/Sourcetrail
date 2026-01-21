#include "ChatController.hpp"

#include <utility>


ChatController::ChatController() noexcept = default;

ChatController::~ChatController() noexcept = default;

void ChatController::onSendMessage(const QString& message) {
  // getView<ChatView>()->displayMessage(message);
}

void ChatController::onResponseReceived(const QString& response) {
  // getView<ChatView>()->displayResponse(response);
}
