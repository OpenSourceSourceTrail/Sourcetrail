#pragma once
#include <QStyledItemDelegate>

/**
 * @brief Custom delegate for rendering chat messages in QListView
 *
 * Renders MessageBubbleWidget for each message in the chat list.
 * Handles sizing and interaction with the model data.
 */
class MessageBubbleDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit MessageBubbleDelegate(QObject* parent = nullptr);
  ~MessageBubbleDelegate() override = default;

  MessageBubbleDelegate(const MessageBubbleDelegate&) = delete;
  MessageBubbleDelegate& operator=(const MessageBubbleDelegate&) = delete;
  MessageBubbleDelegate(MessageBubbleDelegate&&) = delete;
  MessageBubbleDelegate& operator=(MessageBubbleDelegate&&) = delete;

  // QAbstractItemDelegate interface
  [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
