#pragma once
#include <QFrame>

class MessageBubble : public QFrame {
  Q_OBJECT
public:
  enum class Role : uint8_t { User, Assistant };

  MessageBubble(const QString& text, Role role, QWidget* parent = nullptr);

private:
  void setupUI(const QString& text);

  Role m_role;
};
