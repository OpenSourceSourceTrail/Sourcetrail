#include "qt/element/model/RecentItemModel.hpp"

#include <algorithm>
#include <ranges>

#include <QMessageBox>
#include <QMimeData>

#include "project/messages/MessageLoadProject.h"
#include "RangesTo.hpp"
#include "settings/ProjectSettings.h"


namespace {

constexpr const char* MIME_TYPE = "application/x-recent-item-model";

QIcon getProjectIcon(LanguageType lang) {
  static const auto CppIcon = QIcon(":/data/gui/icon/cpp_icon.png");
  static const auto CIcon = QIcon(":/data/gui/icon/c_icon.png");
  static const auto ProjectIcon = QIcon(":/data/gui/icon/empty_icon.png");

  switch(lang) {
  case LanguageType::LANGUAGE_C:
    return CIcon;
  case LanguageType::LANGUAGE_CPP:
    return CppIcon;
  case LanguageType::LANGUAGE_CUSTOM:
  default:
    return ProjectIcon;
  }
}

bool showMissingProjectDialog(const std::wstring& projectFilePath) {
  QMessageBox msgBox;
  msgBox.setText(QStringLiteral("Missing Project File"));
  msgBox.setInformativeText(QString::fromStdWString(L"<p>Couldn't find \"" + projectFilePath +
                                                    L"\" on your filesystem.</p><p>Do you want to remove it from recent project "
                                                    L"list?</p>"));
  msgBox.addButton(QStringLiteral("Remove"), QMessageBox::ButtonRole::YesRole);
  msgBox.addButton(QStringLiteral("Keep"), QMessageBox::ButtonRole::NoRole);
  msgBox.setIcon(QMessageBox::Icon::Question);
  msgBox.exec();
  // exec()'s return value is opaque for custom buttons; the role is the documented way.
  return msgBox.buttonRole(msgBox.clickedButton()) == QMessageBox::ButtonRole::YesRole;
}

}    // namespace

namespace qt::element::model {

RecentItemModel::RecentItemModel(const std::vector<std::filesystem::path>& recentProjects, size_t maxRecentProjects, QObject* parent)
    : QAbstractListModel(parent), mMaxRecentProjects(maxRecentProjects) {
  mRecentProjects = recentProjects | std::views::transform([](const std::filesystem::path& recentProject) -> RecentItem {
                      const auto recentProjectPath = FilePath{recentProject.wstring()};
                      const auto lang = ProjectSettings::getLanguageOfProject(recentProjectPath);
                      std::error_code errorCode;
                      return {std::filesystem::exists(recentProject, errorCode),
                              QString::fromStdWString(recentProjectPath.withoutExtension().fileName()),
                              getProjectIcon(lang),
                              recentProject};
                    }) |
      utility::toContainer<std::vector<RecentItem>>();

  if(mRecentProjects.size() > mMaxRecentProjects) {
    mRecentProjects.resize(mMaxRecentProjects);
  }
}

QStringList RecentItemModel::mimeTypes() const {
  return {MIME_TYPE};
}

QMimeData* RecentItemModel::mimeData(const QModelIndexList& indexes) const {
  if(indexes.count() <= 0) {
    return nullptr;
  }
  const auto types = mimeTypes();
  if(types.isEmpty()) {
    return nullptr;
  }

  const auto row = indexes.front().row();
  if(row < 0 || static_cast<size_t>(row) >= mRecentProjects.size()) {
    return nullptr;
  }

  auto* data = new QMimeData;    // NOLINT(cppcoreguidelines-owning-memory): Qt will take ownership of the pointer
  const auto& format = types.front();
  QByteArray encoded;
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
  QDataStream stream{&encoded, QDataStream::OpenMode::enum_type::WriteOnly};
#else
  QDataStream stream{&encoded, QIODevice::WriteOnly};
#endif

  stream << mRecentProjects[static_cast<size_t>(row)];

  data->setData(format, encoded);

  return data;
}

bool RecentItemModel::canDropMimeData(const QMimeData* data,
                                      [[maybe_unused]] Qt::DropAction action,
                                      [[maybe_unused]] int row,
                                      int column,
                                      [[maybe_unused]] const QModelIndex& parent) const {
  if(!data->hasFormat(MIME_TYPE)) {
    return false;
  }
  if(column > 0) {
    return false;
  }
  return true;
}

bool RecentItemModel::dropMimeData(const QMimeData* data,
                                   [[maybe_unused]] Qt::DropAction action,
                                   int row,
                                   [[maybe_unused]] int column,
                                   const QModelIndex& parent) {
  int beginRow;
  if(row != -1) {
    beginRow = row;
  } else if(parent.isValid()) {
    beginRow = parent.row();
  } else {
    beginRow = rowCount(QModelIndex{});
  }
  beginRow = std::clamp(beginRow, 0, rowCount(QModelIndex{}));

  QByteArray encodedData = data->data(MIME_TYPE);
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
  QDataStream stream{&encodedData, QDataStream::OpenMode::enum_type::ReadOnly};
#else
  QDataStream stream{&encodedData, QIODevice::ReadOnly};
#endif
  RecentItem item;
  stream >> item;

  insertRows(beginRow, 1, QModelIndex());
  QModelIndex idx = index(beginRow, 0, QModelIndex());
  return setData(idx, QVariant::fromValue(item), Qt::EditRole);
}

QVariant RecentItemModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid()) {
    return {};
  }

  const auto idx = static_cast<size_t>(index.row());
  if(idx >= mRecentProjects.size()) {
    return {};
  }

  const auto& item = mRecentProjects[idx];
  switch(role) {
  case Qt::DisplayRole:
    return item.title;
  case Qt::DecorationRole:
    return item.icon;
  case Qt::ToolTipRole:
    return item.exists ? QVariant{} : QString("\"%0\" is missed").arg(QString::fromStdWString(item.path.wstring()));
  case Qt::FontRole: {
    QFont font;
    font.setStrikeOut(true);
    return item.exists ? QVariant{} : font;
  }
  default:
    break;
  }

  return {};
}

bool RecentItemModel::setData(const QModelIndex& index, const QVariant& value, [[maybe_unused]] int role) {
  const auto row = index.row();
  if(!index.isValid() || row < 0 || static_cast<size_t>(row) >= mRecentProjects.size()) {
    return false;
  }
  if(!value.canConvert<RecentItem>()) {
    return false;
  }

  mRecentProjects[static_cast<size_t>(row)] = value.value<RecentItem>();
  emit dataChanged(index, index);
  mDirty = true;
  return true;
}

bool RecentItemModel::insertRows(int row, int count, const QModelIndex& /*parent*/) {
  if(row < 0 || count <= 0 || static_cast<size_t>(row) > mRecentProjects.size()) {
    return false;
  }

  beginInsertRows(QModelIndex(), row, row + count - 1);
  mRecentProjects.insert(mRecentProjects.begin() + row, static_cast<size_t>(count), {});
  endInsertRows();
  return true;
}

bool RecentItemModel::removeRows(int row, int count, const QModelIndex& /*parent*/) {
  if(row < 0 || count <= 0 || static_cast<size_t>(row) + static_cast<size_t>(count) > mRecentProjects.size()) {
    return false;
  }

  beginRemoveRows(QModelIndex(), row, row + count - 1);
  auto begin = std::begin(mRecentProjects) + row;
  auto end = begin + count;
  mRecentProjects.erase(begin, end);
  endRemoveRows();
  return true;
}

void RecentItemModel::clicked(const QModelIndex& index) {
  if(!index.isValid()) {
    return;
  }

  const auto idx = static_cast<size_t>(index.row());
  if(idx >= mRecentProjects.size()) {
    return;
  }

  const auto& item = mRecentProjects[idx];
  if(item.exists) {
    MessageLoadProject(FilePath{item.path.wstring()}).dispatch();
    return;
  }
  if(::showMissingProjectDialog(item.path.wstring())) {
    removeRows(index.row(), 1);
    mDirty = true;
  }
}

}    // namespace qt::element::model
