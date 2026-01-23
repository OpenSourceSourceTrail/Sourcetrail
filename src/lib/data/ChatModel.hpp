#pragma once
#include <vector>

#include <QAbstractListModel>

#include <nonstd/expected.hpp>
#include <qtclasshelpermacros.h>

#include "ChatMessage.hpp"

enum class ChatError : uint8_t { EmptyMessage, ModelNotReady };

/**
 * @brief Standard Qt model for chat message data
 *
 * Follows Qt's model/view architecture. All state changes emit
 * appropriate signals for view synchronization.
 */
class ChatModel final : public QAbstractListModel {
  Q_OBJECT

public:
  enum class Roles : int { ContentRole = Qt::UserRole + 1, RoleRole, TimestampRole };

  explicit ChatModel(QObject* parent = nullptr);
  Q_DISABLE_COPY_MOVE(ChatModel)
  ~ChatModel() override;

  // QAbstractItemModel interface
  [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex{}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  // Business operations
  [[nodiscard]] nonstd::expected<void, ChatError> addMessage(ChatMessage message);
  void clear();

  [[nodiscard]] bool isEmpty() const noexcept {
    return m_messages.empty();
  }
  [[nodiscard]] std::size_t messageCount() const noexcept {
    return m_messages.size();
  }

signals:
  void messageAdded(const ChatMessage& message);
  void messagesCleared();

private:
  std::vector<ChatMessage> m_messages;
};

Q_DECLARE_METATYPE(MessageRole)
