#include "search/ActionModel.h"

#include <utility>

#include <QVariant>

namespace search {

ActionModel::ActionModel(QObject* parent) : QAbstractListModel(parent) {}

ActionModel::~ActionModel() = default;

void ActionModel::setActions(QList<ActionItem> actions) {
  beginResetModel();
  mActions = std::move(actions);
  endResetModel();
}

void ActionModel::run(int row) const {
  if(row < 0 || row >= mActions.size()) {
    return;
  }
  if(const auto& action = mActions.at(row).run) {
    action();
  }
}

QList<int> ActionModel::filter(const QString& query) const {
  QList<int> rows;
  for(int row = 0; row < static_cast<int>(mActions.size()); ++row) {
    if(query.isEmpty() || mActions.at(row).label.contains(query, Qt::CaseInsensitive)) {
      rows.append(row);
    }
  }
  return rows;
}

int ActionModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(mActions.size());
}

QVariant ActionModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() < 0 || index.row() >= mActions.size()) {
    return {};
  }
  const ActionItem& action = mActions.at(index.row());
  switch(role) {
  case IdRole:
    return action.id;
  case LabelRole:
    return action.label;
  case GlyphRole:
    return action.glyph;
  case ShortcutRole:
    return action.shortcut;
  default:
    return {};
  }
}

QHash<int, QByteArray> ActionModel::roleNames() const {
  return {
      {IdRole, "actionId"},
      {LabelRole, "actionLabel"},
      {GlyphRole, "actionGlyph"},
      {ShortcutRole, "actionShortcut"},
  };
}

}    // namespace search
