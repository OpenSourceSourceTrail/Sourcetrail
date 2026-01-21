#pragma once
#include <QFrame>
#include <QtCore>
#include <QWidget>

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

private:
  void sendMessage();

  void clearChat();

  void setupUI();

  QWidget* createHeader();

  QWidget* createInputArea();

  [[nodiscard]] QString generateResponse(const QString& query) const;

  void applyStyles();

  QWidget* m_widget{nullptr};
  QVBoxLayout* m_chatLayout{nullptr};
  QLineEdit* m_inputField{nullptr};
};
