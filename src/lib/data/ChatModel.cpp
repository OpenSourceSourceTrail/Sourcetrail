#include "ChatModel.hpp"

ChatModel::ChatModel(QObject* parent) : QAbstractListModel{parent} {
  m_messages.reserve(100);    // Reasonable default capacity
}

ChatModel::~ChatModel() = default;

int ChatModel::rowCount([[maybe_unused]] const QModelIndex& parent) const {
  return static_cast<int>(m_messages.size());
}

QVariant ChatModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() >= static_cast<int>(m_messages.size())) {
    return {};
  }

  const auto& message = m_messages[static_cast<std::size_t>(index.row())];

  switch(static_cast<Roles>(role)) {
  case Roles::ContentRole:
    return message.content();
  case Roles::RoleRole:
    return QVariant::fromValue(message.role());
  case Roles::TimestampRole:
    return message.timestamp();
  default:
    return {};
  }
}

QHash<int, QByteArray> ChatModel::roleNames() const {
  return {{static_cast<int>(Roles::ContentRole), "content"},
          {static_cast<int>(Roles::RoleRole), "messageRole"},
          {static_cast<int>(Roles::TimestampRole), "timestamp"}};
}

nonstd::expected<void, ChatError> ChatModel::addMessage(ChatMessage message) {
  if(message.content().trimmed().isEmpty()) {
    return nonstd::unexpected{ChatError::EmptyMessage};
  }

  const int newRow = static_cast<int>(m_messages.size());

  beginInsertRows(QModelIndex{}, newRow, newRow);
  m_messages.push_back(std::move(message));
  endInsertRows();

  emit messageAdded(m_messages.back());

  return {};
}

void ChatModel::clear() {
  if(m_messages.empty()) {
    return;
  }

  beginResetModel();
  m_messages.clear();
  endResetModel();

  emit messagesCleared();
}
