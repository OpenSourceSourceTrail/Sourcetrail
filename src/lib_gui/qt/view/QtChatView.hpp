#pragma once
#include <cstddef>
#include <memory>

#include <QFrame>
#include <QtCore>
#include <QWidget>

#include "ChatView.hpp"
#include "MessageBubble.hpp"


class QVBoxLayout;
class QLineEdit;

class QtChatView : public ChatView {
public:
  explicit QtChatView(ViewLayout* viewLayout) noexcept;
  Q_DISABLE_COPY_MOVE(QtChatView)
  ~QtChatView() override;

  [[nodiscard]] std::string getName() const override {
    return "Chat View";
  }

  void createWidgetWrapper() override {}
  void refreshView() override {}

private slots:
  void sendMessage();

  void clearChat();

private:
  void setupUI();

  QWidget* createHeader();

  QWidget* createInputArea();

  void addMessage(const QString& text, MessageBubble::Role role);

  [[nodiscard]] QString generateResponse(const QString& query) const;

  void applyStyles();

  QWidget* m_widget = nullptr;
  QVBoxLayout* m_chatLayout{nullptr};
  QLineEdit* m_inputField{nullptr};
};
