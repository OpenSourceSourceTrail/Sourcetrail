#pragma once
#include <memory>

#include <QFrame>
#include <QWidget>

#include <qtclasshelpermacros.h>

#include "ChatView.hpp"

class QVBoxLayout;
class QScrollArea;
class QLineEdit;
class ChatModel;
class ChatMessage;

/**
 * @brief Qt implementation of ChatView following strict MVC pattern
 *
 * Responsibilities:
 * - Render data from ChatModel
 * - Forward user interactions to controller via signals
 * - No business logic
 * - All widgets owned through Qt parent-child or unique_ptr
 *
 * This view is fully testable through the ChatModel interface.
 */
class QtChatView final : public ChatView {
  Q_OBJECT

public:
  explicit QtChatView(ViewLayout* viewLayout, std::shared_ptr<ChatModel> model = std::make_shared<ChatModel>());
  Q_DISABLE_COPY_MOVE(QtChatView)
  ~QtChatView() override;

  // ChatView interface - pure presentation updates
  void setInputEnabled(bool enabled) override;
  void clearInput() override;
  void focusInput() override;

signals:
  // Forward user actions to controller
  void messageSubmitted(const QString& content);
  void clearRequested();

private slots:
  void onMessageAdded(const ChatMessage& message);
  void onMessagesCleared();

private:
  void setupUI();
  void setupConnections();
  void loadStyleSheet();

  [[nodiscard]] QWidget* createHeader();
  [[nodiscard]] QWidget* createChatArea();
  [[nodiscard]] QWidget* createInputArea();

  void scrollToBottom();
  void handleSubmit();

  std::shared_ptr<ChatModel> mModel;
  std::unique_ptr<QWidget> mWidget;

  // Non-owning pointers to widgets owned by Qt parent-child
  QVBoxLayout* mChatLayout{nullptr};
  QScrollArea* mScrollArea{nullptr};
  QLineEdit* mInputField{nullptr};
};
