#pragma once
#include <QFrame>
#include <QWidget>

#include <qobject.h>

#include "ChatView.hpp"


class QVBoxLayout;
class QLineEdit;

class QtChatView final : public ChatView {
public:
  explicit QtChatView(ViewLayout* viewLayout) noexcept;
  Q_DISABLE_COPY_MOVE(QtChatView)
  ~QtChatView() override;

  void createWidgetWrapper() override {}

  void refreshView() override {}

  void addMessage(const QString& text, Role role) override;

  void setInputEnabled(bool enabled) override;

  void clearInput() override;

  void clearChat() override;

private:
  void sendMessage();

  void setupUI();

  QWidget* createHeader();

  QWidget* createInputArea();

  // TODO(Hussein): Port to resource file
  void applyStyles();

  QWidget* m_widget{nullptr};
  QVBoxLayout* m_chatLayout{nullptr};
  QLineEdit* m_inputField{nullptr};
};
