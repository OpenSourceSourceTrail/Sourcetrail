#include "search/MatchModel.h"

#include <utility>

#include <QVariant>

namespace search {

MatchModel::MatchModel(QObject* parent) : QAbstractListModel(parent) {}

MatchModel::~MatchModel() = default;

void MatchModel::setItems(QList<MatchItem> items) {
  // The list is replaced wholesale on every keystroke and is at most a screenful; a diff would
  // cost more than it saves.
  beginResetModel();
  mItems = std::move(items);
  endResetModel();
}

const QList<MatchItem>& MatchModel::items() const {
  return mItems;
}

int MatchModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(mItems.size());
}

QVariant MatchModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() < 0 || index.row() >= mItems.size()) {
    return {};
  }
  const MatchItem& item = mItems.at(index.row());
  switch(role) {
  case NameRole:
    return item.name;
  case SubtextRole:
    return item.subtext;
  case TypeNameRole:
    return item.typeName;
  case NodeTypeRole:
    return item.nodeType;
  case SearchTypeRole:
    return item.searchType;
  case IndicesRole:
    return QVariant::fromValue(item.indices);
  case HasChildrenRole:
    return item.hasChildren;
  default:
    return {};
  }
}

QHash<int, QByteArray> MatchModel::roleNames() const {
  return {
      {NameRole, "matchName"},
      {SubtextRole, "matchSubtext"},
      {TypeNameRole, "matchTypeName"},
      {NodeTypeRole, "matchNodeType"},
      {SearchTypeRole, "matchSearchType"},
      {IndicesRole, "matchIndices"},
      {HasChildrenRole, "matchHasChildren"},
  };
}

}    // namespace search
