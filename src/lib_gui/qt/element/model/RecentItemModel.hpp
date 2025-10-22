#pragma once
#include <filesystem>
#include <vector>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include <QAbstractListModel>
#include <QIcon>

namespace qt::element::model {

struct RecentItem final {
  bool exists;
  QString title;
  QIcon icon;
  std::filesystem::path path;
};

inline QDataStream& operator<<(QDataStream& str, const RecentItem& item) {
  str << item.exists << item.title << item.icon << QString::fromStdWString(item.path.wstring());
  return str;
}

inline QDataStream& operator>>(QDataStream& str, RecentItem& item) {
  QString path;
  str >> item.exists >> item.title >> item.icon >> path;
  item.path = path.toStdWString();
  return str;
}

struct RecentItemModel final : QAbstractListModel {
  RecentItemModel(const std::vector<std::filesystem::path>& recentProjects, size_t maxRecentProjects, QObject* parent = nullptr);

  [[nodiscard]] QStringList mimeTypes() const override;

  [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;

  [[nodiscard]] bool canDropMimeData(
      const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

  bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

  [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  bool setData(const QModelIndex& index, const QVariant& value, int role) override;

  [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override {
    std::ignore = parent;
    return static_cast<int>(std::min(mMaxRecentProjects, mRecentProjects.size()));
  }

  void clicked(const QModelIndex& index);

  bool insertRows(int row, int rows, const QModelIndex& parent = QModelIndex()) override;

  void removeItem(int index) {
    if(static_cast<size_t>(index) >= mRecentProjects.size()) {
      return;
    }

    mRecentProjects.erase(std::begin(mRecentProjects) + index);
    updateRecentProjects();
  }

  bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

  [[nodiscard]] Qt::DropActions supportedDropActions() const override {
    return Qt::CopyAction | Qt::MoveAction;
  }

  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override {
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

    if(index.isValid()) {
      return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    } else {
      return Qt::ItemIsDropEnabled | defaultFlags;
    }
  }

  [[nodiscard]] bool isDirty() const {
    return mDirty;
  }

  void clearDirty() {
    mDirty = false;
  }

  [[nodiscard]] const std::vector<RecentItem>& getRecentProjects() const {
    return mRecentProjects;
  }

private:
  bool mDirty = false;
  size_t mMaxRecentProjects = 0;
  std::vector<RecentItem> mRecentProjects;
  void updateRecentProjects();
};

}    // namespace qt::element::model

Q_DECLARE_METATYPE(qt::element::model::RecentItem)