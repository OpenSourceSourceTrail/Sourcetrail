#pragma once
#include <QFrame>

#include "ChatMessage.hpp"

class MessageBubble : public QFrame {
  Q_OBJECT
public:
  MessageBubble(const QString& text, MessageRole role, QWidget* parent = nullptr);

private:
  void setupUI(const QString& text);

  MessageRole mRole;
};
