#include "MessageBubbleDelegate.hpp"

#include <QPainter>

#include <qlogging.h>

#include "ChatMessage.hpp"
#include "ChatModel.hpp"
#include "MessageBubbleWidget.hpp"


namespace {
ChatMessage extractMessageFromIndex(const QModelIndex& index) {
  const auto* model = qobject_cast<const ChatModel*>(index.model());
  if(model == nullptr) {
    return ChatMessage{QString{}, MessageRole::Error};
  }

  // Extract data from model roles
  const QString content = model->data(index, static_cast<int>(ChatModel::Roles::ContentRole)).toString();
  const auto role = model->data(index, static_cast<int>(ChatModel::Roles::RoleRole)).value<MessageRole>();
  const auto timestamp = QDateTime::fromString(
      model->data(index, static_cast<int>(ChatModel::Roles::TimestampRole)).toString(), Qt::ISODate);

  return ChatMessage{content, role, timestamp};
}
}    // namespace


MessageBubbleDelegate::MessageBubbleDelegate(QObject* parent) : QStyledItemDelegate{parent} {}

QSize MessageBubbleDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  // Extract message data from model
  const ChatMessage message = extractMessageFromIndex(index);

  // Create a temporary widget to measure size
  auto widget = std::make_unique<MessageBubbleWidget>(message);
  widget->adjustSize();

  // Return the widget's size with some padding
  constexpr int padding = 12;
  return QSize{option.rect.width(), widget->sizeHint().height() + padding};
}

void MessageBubbleDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  // Extract message data from model
  const ChatMessage message = extractMessageFromIndex(index);

  // Save painter state
  painter->save();

  // Set background color based on role
  QColor backgroundColor;
  switch(message.role()) {
  case MessageRole::User:
    backgroundColor = QColor("#007ACC");
    break;
  case MessageRole::Assistant:
    backgroundColor = QColor("#2D2D30");
    break;
  case MessageRole::Error:
    backgroundColor = QColor("#5A1D1D");
    break;
  default:
    backgroundColor = QColor("#3E3E42");
    break;
  }

  // Draw background
  painter->setBrush(backgroundColor);
  painter->setPen(Qt::NoPen);
  painter->drawRoundedRect(option.rect, 8, 8);

  // Set text color
  QColor textColor = (message.role() == MessageRole::User) ? QColor("white") : QColor("#E3E3E3");
  painter->setPen(textColor);

  // Draw content
  QRect contentRect = option.rect.adjusted(12, 8, -12, -24);
  painter->drawText(contentRect, Qt::TextWordWrap, message.content());

  // Draw timestamp
  QRect timestampRect = option.rect.adjusted(12, option.rect.height() - 20, -12, -8);
  painter->setFont(QFont(painter->font().family(), 9));
  painter->setPen(QColor("#858585"));
  painter->drawText(timestampRect, Qt::AlignRight, message.timestamp().toString("hh:mm"));

  // Restore painter state
  painter->restore();
}
