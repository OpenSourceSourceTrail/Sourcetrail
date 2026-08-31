#pragma once
#include <QAbstractListModel>

#include "shell/ShellSnapshot.h"

namespace shell {

/**
 * The navigation history, as the back/forward menu lists it.
 *
 * Deliberately not registered with QML -- it reaches the scene as a QAbstractItemModel* on
 * NavigationViewModel. Registering it would let QML construct one, and QML_UNCREATABLE still
 * generates a creator that will not compile against a final class.
 */
class HistoryModel final : public QAbstractListModel {
  Q_OBJECT

public:
  enum Role : int {
    NameRole = Qt::UserRole + 1,
    TypeNameRole,
    NodeTypeRole,
    IsCurrentRole,
  };

  explicit HistoryModel(QObject* parent = nullptr);
  ~HistoryModel() override;

  void setItems(const QList<HistoryItem>& items);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
  QList<HistoryItem> mItems;
};

}    // namespace shell
