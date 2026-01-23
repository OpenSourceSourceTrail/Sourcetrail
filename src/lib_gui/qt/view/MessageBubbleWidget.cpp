#include "MessageBubbleWidget.hpp"

#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>


MessageBubbleWidget::MessageBubbleWidget(const ChatMessage& message, QWidget* parent) : QFrame{parent} {
  setupUI(message);
  applyRoleStyles(message.role());
}

void MessageBubbleWidget::setupUI(const ChatMessage& message) {
  setFrameShape(QFrame::StyledPanel);
  setProperty("role", roleToStyleClass(message.role()));

  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(12, 8, 12, 8);
  layout->setSpacing(4);

  mContentLabel = new QLabel{message.content(), this};
  mContentLabel->setWordWrap(true);
  mContentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
  mContentLabel->setObjectName("contentLabel");

  mTimestampLabel = new QLabel{message.timestamp().toString("hh:mm"), this};
  mTimestampLabel->setObjectName("timestampLabel");

  QFont timestampFont = mTimestampLabel->font();
  timestampFont.setPointSize(9);
  mTimestampLabel->setFont(timestampFont);

  layout->addWidget(mContentLabel);
  layout->addWidget(mTimestampLabel, 0, Qt::AlignRight);
}

void MessageBubbleWidget::updateContent(const ChatMessage& message) {
  mContentLabel->setText(message.content());
  mTimestampLabel->setText(message.timestamp().toString("hh:mm"));
  applyRoleStyles(message.role());
}

void MessageBubbleWidget::applyRoleStyles(MessageRole role) {
  setProperty("role", roleToStyleClass(role));
  style()->unpolish(this);
  style()->polish(this);
}

QString MessageBubbleWidget::roleToStyleClass(MessageRole role) noexcept {
  switch(role) {
  case MessageRole::User:
    return "user";
  case MessageRole::Assistant:
    return "assistant";
  case MessageRole::System:
    return "system";
  case MessageRole::Error:
    return "error";
  }
  return "system";
}
