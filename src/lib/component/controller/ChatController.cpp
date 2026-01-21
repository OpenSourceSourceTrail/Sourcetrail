#include "ChatController.hpp"

#include <QTimer>

#include "ChatView.hpp"

ChatController::ChatController() noexcept = default;

ChatController::~ChatController() noexcept = default;

void ChatController::sendMessage(const QString& message) {
  emit messageToAdd(message, Role::User);
  emit clearInputRequested();
  emit inputStateChanged(false);
  // TESTING: Simulate LLM response after delay
  if(message.contains("error")) {
    onErrorOccurred("An error occurred while processing your request.");
  }
  QTimer::singleShot(1000, this, [this, message]() {
    // Simulate response from LLM
    onResponseReceived("This is a simulated response to your message: " + message);
  });
}

void ChatController::onResponseReceived(const QString& message) {
  emit messageToAdd(message, Role::Assistant);
  emit inputStateChanged(true);
}

void ChatController::onErrorOccurred(const QString& error) {
  QString errorMsg = tr("Error: %1").arg(error);
  emit errorOccurred(errorMsg);
  emit inputStateChanged(true);
}
