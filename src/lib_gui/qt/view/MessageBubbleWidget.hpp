#pragma once
#include <memory>

#include <QFrame>

#include "ChatMessage.hpp"


class QLabel;

/**
 * @brief Visual representation of a single chat message
 *
 * Pure presentation component with no business logic.
 * Owns its child widgets through Qt's parent-child hierarchy.
 */
class MessageBubbleWidget final : public QFrame {
  Q_OBJECT

public:
  explicit MessageBubbleWidget(const ChatMessage& message, QWidget* parent = nullptr);
  ~MessageBubbleWidget() override = default;

  MessageBubbleWidget(const MessageBubbleWidget&) = delete;
  MessageBubbleWidget& operator=(const MessageBubbleWidget&) = delete;
  MessageBubbleWidget(MessageBubbleWidget&&) = delete;
  MessageBubbleWidget& operator=(MessageBubbleWidget&&) = delete;

  void updateContent(const ChatMessage& message);

private:
  void setupUI(const ChatMessage& message);
  void applyRoleStyles(MessageRole role);
  [[nodiscard]] static QString roleToStyleClass(MessageRole role) noexcept;

  QLabel* mContentLabel{nullptr};       // Owned by Qt parent-child
  QLabel* m_timestampLabel{nullptr};    // Owned by Qt parent-child
};
