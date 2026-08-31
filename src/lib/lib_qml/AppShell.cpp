#include "AppShell.h"

#include <QJSEngine>
#include <QQmlEngine>

#include "app/Application.h"
#include "component/controller/ActivationController.h"
#include "data/storage/StorageCache.h"
#include "graph/GraphViewModel.h"
#include "GuiThread.h"
#include "QmlDialogView.h"
#include "search/SearchViewModel.h"
#include "settings/IApplicationSettings.hpp"
#include "shell/NavigationViewModel.h"
#include "shell/StatusViewModel.h"
#include "ShellMessages.h"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "type/MessageQuitApplication.h"
#include "type/MessageRefresh.h"
#include "utilityString.h"

AppShell* AppShell::sInstance = nullptr;

AppShell::AppShell(QObject* parent) : QObject(parent) {
  sInstance = this;
}

AppShell::~AppShell() {
  sInstance = nullptr;
}

AppShell* AppShell::create(QQmlEngine* /*qmlEngine*/, QJSEngine* jsEngine) {
  // main() owns the shell -- Application holds a raw pointer to it for the whole run -- so the QML
  // engine must not delete it when the last reference goes away.
  QJSEngine::setObjectOwnership(sInstance, QJSEngine::CppOwnership);
  Q_UNUSED(jsEngine)
  return sInstance;
}

QString AppShell::title() const {
  return mTitle;
}

QString AppShell::status() const {
  return mStatus;
}

bool AppShell::statusIsError() const {
  return mStatusIsError;
}

bool AppShell::projectLoaded() const {
  return mProjectLoaded;
}

bool AppShell::indexing() const {
  return mIndexing;
}

QString AppShell::progressMessage() const {
  return mProgressMessage;
}

int AppShell::progressPercent() const {
  return mProgressPercent;
}

QStringList AppShell::recentProjects() const {
  return mRecentProjects;
}

QString AppShell::projectSummary() const {
  return mProjectSummary;
}

int AppShell::uiFontSize() const {
  return IApplicationSettings::getInstanceRaw()->getFontSize();
}

void AppShell::setUiFontSize(int pointSize) {
  auto* settings = IApplicationSettings::getInstanceRaw();
  if(settings->getFontSize() == pointSize) {
    return;
  }
  settings->setFontSize(pointSize);
  settings->save();
  Q_EMIT uiFontSizeChanged();
}

void AppShell::loadProject(const QUrl& projectFile) {
  const auto path = projectFile.isLocalFile() ? projectFile.toLocalFile() : projectFile.toString();
  MessageLoadProject{FilePath{path.toStdString()}, false, RefreshMode::None}.dispatch();
}

void AppShell::refresh(bool all) {
  MessageRefresh message;
  if(all) {
    message.refreshAll();
  }
  message.dispatch();
}

void AppShell::cancelIndexing() {
  MessageIndexingInterrupted{}.dispatch();
}

void AppShell::quit() {
  MessageQuitApplication{}.dispatch();
}

void AppShell::setup() {
  // Runs inside Application::createInstance, so the message queue exists by now but the QML engine
  // does not yet -- nothing here may touch the scene.
  mMessages = std::make_unique<ShellMessages>(this);

  auto* storageAccess = Application::getInstance()->getStorageCache();
  mActivation = std::make_unique<ActivationController>(storageAccess);
  mGraph = std::make_unique<graph::GraphViewModel>(nullptr);
  mGraph->attach(storageAccess);
  mNavigation = std::make_unique<shell::NavigationViewModel>(nullptr);
  mNavigation->attach(storageAccess);
  mStatusModel = std::make_unique<shell::StatusViewModel>(nullptr);
  mStatusModel->attach(storageAccess);
  mSearch = std::make_unique<search::SearchViewModel>(nullptr);
  mSearch->attach(storageAccess);

  updateRecentProjectMenu();
}

void AppShell::saveLayout() {}

void AppShell::clear() {
  setProjectLoaded(false);
}

std::shared_ptr<DialogView> AppShell::getDialogView(DialogView::UseCase useCase) {
  return std::make_shared<QmlDialogView>(useCase, Application::getInstance()->getStorageCache(), this);
}

void AppShell::refreshViews() {
  qml::postToGui(this, [this]() { Q_EMIT viewsNeedRefresh(false); });
}

void AppShell::refreshUIState(bool isAfterIndexing) {
  const bool loaded = Application::getInstance()->isProjectLoaded();

  // Read the counts here, on the bus thread: this is a database query, and the whole point of the
  // hop below is that the GUI thread never makes one.
  QString summary;
  if(loaded) {
    const auto stats = Application::getInstance()->getStorageCache()->getStorageStats();
    summary = QStringLiteral("%1 symbols, %2 references, %3/%4 files, %5 lines")
                  .arg(stats.nodeCount)
                  .arg(stats.edgeCount)
                  .arg(stats.completedFileCount)
                  .arg(stats.fileCount)
                  .arg(stats.fileLOCCount);
  }

  qml::postToGui(this, [this, loaded, isAfterIndexing, summary = std::move(summary)]() {
    setProjectLoaded(loaded);
    if(mProjectSummary != summary) {
      mProjectSummary = summary;
      Q_EMIT projectSummaryChanged();
    }
    Q_EMIT viewsNeedRefresh(isAfterIndexing);
  });
}

void AppShell::loadWindow(bool showStartWindow) {
  qml::postToGui(this, [this, showStartWindow]() { Q_EMIT startScreenRequested(showStartWindow); });
}

void AppShell::hideStartScreen() {
  qml::postToGui(this, [this]() { Q_EMIT startScreenRequested(false); });
}

void AppShell::activateWindow() {
  qml::postToGui(this, [this]() { Q_EMIT windowActivationRequested(); });
}

void AppShell::setTitle(const std::wstring& title) {
  auto value = QString::fromStdString(utility::encodeToUtf8(title));
  qml::postToGui(this, [this, value = std::move(value)]() {
    if(mTitle != value) {
      mTitle = value;
      Q_EMIT titleChanged();
    }
  });
}

void AppShell::updateRecentProjectMenu() {
  QStringList projects;
  for(const auto& path : IApplicationSettings::getInstanceRaw()->getRecentProjects()) {
    projects.append(QString::fromStdString(path.string()));
  }

  qml::postToGui(this, [this, projects = std::move(projects)]() {
    if(mRecentProjects != projects) {
      mRecentProjects = projects;
      Q_EMIT recentProjectsChanged();
    }
  });
}

void AppShell::updateHistoryMenu(std::shared_ptr<MessageBase> /*message*/) {
  // The history view-model listens on the bus directly; nothing to mirror here yet.
}

void AppShell::updateBookmarksMenu(const std::vector<std::shared_ptr<Bookmark>>& /*bookmarks*/) {
  // Likewise the bookmark view-model.
}

void AppShell::reportProgress(const QString& message, int percent) {
  qml::postToGui(this, [this, message, percent]() {
    mIndexing = true;
    mProgressMessage = message;
    mProgressPercent = percent;
    Q_EMIT progressChanged();
  });
}

void AppShell::clearProgress() {
  qml::postToGui(this, [this]() {
    if(!mIndexing) {
      return;
    }
    mIndexing = false;
    mProgressMessage.clear();
    mProgressPercent = 0;
    Q_EMIT progressChanged();
  });
}

void AppShell::reportStatus(const QString& status, bool isError) {
  qml::postToGui(this, [this, status, isError]() {
    mStatus = status;
    mStatusIsError = isError;
    Q_EMIT statusChanged();
  });
}

void AppShell::setProjectLoaded(bool loaded) {
  if(mProjectLoaded == loaded) {
    return;
  }
  mProjectLoaded = loaded;
  Q_EMIT projectLoadedChanged();
}
