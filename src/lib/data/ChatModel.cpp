#include "ChatModel.hpp"

ChatModel::ChatModel(QObject* parent) : QAbstractListModel{parent} {
  // No need to reserve capacity for QStandardItemModel
}

ChatModel::~ChatModel() = default;

int ChatModel::rowCount([[maybe_unused]] const QModelIndex& parent) const {
  return m_messages.rowCount();
}

QVariant ChatModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() >= m_messages.rowCount()) {
    return {};
  }

  const auto* item = m_messages.item(index.row());
  if(item == nullptr) {
    return {};
  }

  switch(static_cast<Roles>(role)) {
  case Roles::ContentRole:
    return item->data(static_cast<int>(Roles::ContentRole));
  case Roles::RoleRole:
    return item->data(static_cast<int>(Roles::RoleRole));
  case Roles::TimestampRole:
    return item->data(static_cast<int>(Roles::TimestampRole));
  default:
    return {};
  }
}

QHash<int, QByteArray> ChatModel::roleNames() const {
  return {{static_cast<int>(Roles::ContentRole), "content"},
          {static_cast<int>(Roles::RoleRole), "messageRole"},
          {static_cast<int>(Roles::TimestampRole), "timestamp"}};
}

nonstd::expected<void, ChatError> ChatModel::addMessage(const ChatMessage& message) {
  if(message.content().trimmed().isEmpty()) {
    return nonstd::unexpected<ChatError>{ChatError::EmptyMessage};
  }

  const int newRow = m_messages.rowCount();

  beginInsertRows(QModelIndex{}, newRow, newRow);
  auto item = std::make_unique<QStandardItem>();
  item->setData(message.content(), static_cast<int>(Roles::ContentRole));
  item->setData(QVariant::fromValue(message.role()), static_cast<int>(Roles::RoleRole));
  item->setData(message.timestamp(), static_cast<int>(Roles::TimestampRole));
  m_messages.appendRow(item.release());
  endInsertRows();

  emit messageAdded(message);

  return {};
}

void ChatModel::clear() {
  if(m_messages.rowCount() == 0) {
    return;
  }

  beginResetModel();
  m_messages.clear();
  endResetModel();

  emit messagesCleared();
}
