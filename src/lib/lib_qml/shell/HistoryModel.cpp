#include "shell/HistoryModel.h"

namespace shell {

HistoryModel::HistoryModel(QObject* parent) : QAbstractListModel(parent) {}

HistoryModel::~HistoryModel() = default;

void HistoryModel::setItems(const QList<HistoryItem>& items) {
  // The history is short and replaced wholesale on every navigation; a diff would cost more than
  // it saves.
  beginResetModel();
  mItems = items;
  endResetModel();
}

int HistoryModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(mItems.size());
}

QVariant HistoryModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() < 0 || index.row() >= mItems.size()) {
    return {};
  }
  const HistoryItem& item = mItems.at(index.row());
  switch(role) {
  case NameRole:
    return item.name;
  case TypeNameRole:
    return item.typeName;
  case NodeTypeRole:
    return item.nodeType;
  case IsCurrentRole:
    return item.isCurrent;
  default:
    return {};
  }
}

QHash<int, QByteArray> HistoryModel::roleNames() const {
  return {
      {NameRole, "historyName"},
      {TypeNameRole, "historyTypeName"},
      {NodeTypeRole, "historyNodeType"},
      {IsCurrentRole, "historyIsCurrent"},
  };
}

}    // namespace shell
