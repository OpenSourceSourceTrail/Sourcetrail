#pragma once
#include <QFrame>

#include "ChatView.hpp"

class MessageBubble : public QFrame {
  Q_OBJECT
public:
  MessageBubble(const QString& text, Role role, QWidget* parent = nullptr);

private:
  void setupUI(const QString& text);

  Role m_role;
};
