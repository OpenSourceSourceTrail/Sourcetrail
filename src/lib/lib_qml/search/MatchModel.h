#pragma once
#include <QAbstractListModel>

#include "search/SearchSnapshot.h"

namespace search {

/**
 * A list of search matches -- the autocompletion feed, or the chips standing for what is active.
 *
 * Deliberately not registered with QML; it reaches the scene as a QAbstractItemModel* on
 * SearchViewModel. See the note in graph/GraphModels.h for why registering it would not compile.
 */
class MatchModel final : public QAbstractListModel {
  Q_OBJECT

public:
  enum Role : int {
    NameRole = Qt::UserRole + 1,
    SubtextRole,
    TypeNameRole,
    NodeTypeRole,
    SearchTypeRole,
    IndicesRole,
    HasChildrenRole,
  };

  explicit MatchModel(QObject* parent = nullptr);
  ~MatchModel() override;

  void setItems(QList<MatchItem> items);
  [[nodiscard]] const QList<MatchItem>& items() const;

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
  QList<MatchItem> mItems;
};

}    // namespace search
