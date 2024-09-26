#include "QtMainWindow.h"

#include <fmt/format.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QLayout>
#include <QMenuBar>
#include <QMouseEvent>
#include <QSettings>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>

#include "Application.h"
#include "Bookmark.h"
#include "CompositeView.h"
#include "FileSystem.h"
#include "MessageActivateBase.h"
#include "MessageActivateOverview.h"
#include "MessageBookmarkActivate.h"
#include "MessageBookmarkBrowse.h"
#include "MessageBookmarkCreate.h"
#include "MessageCloseProject.h"
#include "MessageCodeReference.h"
#include "MessageCustomTrailShow.h"
#include "MessageErrorsHelpMessage.h"
#include "MessageFind.h"
#include "MessageFocusView.h"
#include "MessageHistoryRedo.h"
#include "MessageHistoryUndo.h"
#include "MessageIndexingShowDialog.h"
#include "MessageLoadProject.h"
#include "MessageRefresh.h"
#include "MessageRefreshUI.h"
#include "MessageResetZoom.h"
#include "MessageSaveAsImage.h"
#include "MessageTabClose.h"
#include "MessageTabOpen.h"
#include "MessageTabSelect.h"
#include "MessageWindowClosed.h"
#include "MessageZoom.h"
#include "QtAbout.h"
#include "QtContextMenu.h"
#include "QtFileDialog.h"
#include "QtKeyboardShortcuts.h"
#include "QtLicenseWindow.h"
#include "QtPreferencesWindow.h"
#include "QtProjectWizard.h"
#include "QtStartScreen.hpp"
#include "QtViewWidgetWrapper.h"
#include "ResourcePaths.h"
#include "TabbedView.h"
#include "UserPaths.h"
#include "View.h"
#include "IApplicationSettings.hpp"
#include "globalStrings.h"
#include "logging.h"
#include "tracing.h"
#include "utilityApp.h"
#include "utilityQt.h"
#include "utilityString.h"

QtViewToggle::QtViewToggle(View* view, QWidget* parent) : QWidget(parent), m_view(view) {}

void QtViewToggle::clear() {
  m_view = nullptr;
}

void QtViewToggle::toggledByAction() {
  if(m_view != nullptr) {
    if(auto* window = dynamic_cast<QtMainWindow*>(parent()); window != nullptr) {
      window->toggleView(m_view, true);
    }
  }
}

void QtViewToggle::toggledByUI() {
  if(m_view != nullptr) {
    if(auto* window = dynamic_cast<QtMainWindow*>(parent()); window != nullptr) {
      window->toggleView(m_view, false);
    }
  }
}


MouseReleaseFilter::MouseReleaseFilter(QObject* parent) : QObject(parent) {
  m_backButton = IApplicationSettings::getInstanceRaw()->getControlsMouseBackButton();
  m_forwardButton = IApplicationSettings::getInstanceRaw()->getControlsMouseForwardButton();
}

bool MouseReleaseFilter::eventFilter(QObject* obj, QEvent* event) {
  if(event->type() == QEvent::MouseButtonRelease) {
    if(const auto* mouseEvent = dynamic_cast<QMouseEvent*>(event); mouseEvent != nullptr) {
      if(mouseEvent->button() == m_backButton) {
        MessageHistoryUndo().dispatch();
        return true;
      } else if(mouseEvent->button() == m_forwardButton) {
        MessageHistoryRedo().dispatch();
        return true;
      }
    }
  }

  return QObject::eventFilter(obj, event);
}


QtMainWindow::QtMainWindow()
    : m_historyMenu(nullptr), m_bookmarksMenu(nullptr), m_showDockWidgetTitleBars(true), m_windowStack(this) {
  setObjectName(QStringLiteral("QtMainWindow"));
  setCentralWidget(nullptr);
  setDockNestingEnabled(true);

  setWindowIcon(QIcon(QString::fromStdWString(ResourcePaths::getGuiDirectoryPath().concatenate(L"icon/logo_1024_1024.png").wstr())));
  setWindowFlags(Qt::Widget);

  if(auto* app = dynamic_cast<QApplication*>(QCoreApplication::instance()); app != nullptr) {
    app->installEventFilter(new MouseReleaseFilter(this));

    refreshStyle();

    if(utility::getOsType() != OS_MAC) {
      // can only be done once, because resetting the style on the QCoreApplication causes crash
      app->setStyleSheet(utility::getStyleSheet(ResourcePaths::getGuiDirectoryPath().concatenate(L"main/scrollbar.css")).c_str());
    }
  }

  setupProjectMenu();
  setupEditMenu();
  setupViewMenu();
  setupHistoryMenu();
  setupBookmarksMenu();
  setupHelpMenu();

  // Need to call loadLayout here for right DockWidget size on Linux
  // Second call is in Application.cpp
  loadLayout();
}

QtMainWindow::~QtMainWindow() {
  for(DockWidget& dockWidget : m_dockWidgets) {
    dockWidget.toggle->clear();
  }
}

void QtMainWindow::addView(View* view) {
  const QString name = QString::fromStdString(view->getName());
  if(name == QLatin1String("Tabs")) {
    auto* toolBar = new QToolBar;    // NOLINT(cppcoreguidelines-owning-memory)
    toolBar->setObjectName("Tool" + name);
    toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->setStyleSheet(QStringLiteral("* { margin: 0; }"));
    toolBar->addWidget(QtViewWidgetWrapper::getWidgetOfView(view));
    addToolBar(toolBar);
    return;
  }

  auto* dock = new QDockWidget(name, this);    // NOLINT(cppcoreguidelines-owning-memory)
  dock->setObjectName("Dock" + name);

  dock->setWidget(new QWidget);                      // NOLINT(cppcoreguidelines-owning-memory)
  auto* layout = new QVBoxLayout(dock->widget());    // NOLINT(cppcoreguidelines-owning-memory)
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(QtViewWidgetWrapper::getWidgetOfView(view));

  // Disable un-intended vertical growth of search widget
  if(name == QLatin1String("Search")) {
    dock->setSizePolicy(dock->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
  }

  if(!m_showDockWidgetTitleBars) {
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setTitleBarWidget(new QWidget);    // NOLINT(cppcoreguidelines-owning-memory)
  }

  addDockWidget(Qt::TopDockWidgetArea, dock);

  auto* toggle = new QtViewToggle(view, this);    // NOLINT(cppcoreguidelines-owning-memory)
  connect(dock, &QDockWidget::visibilityChanged, toggle, &QtViewToggle::toggledByUI);

  auto* action = new QAction(name + " Window", this);    // NOLINT(cppcoreguidelines-owning-memory)
  action->setCheckable(true);
  connect(action, &QAction::triggered, toggle, &QtViewToggle::toggledByAction);
  m_viewMenu->insertAction(m_viewSeparator, action);

  DockWidget dockWidget {};
  dockWidget.widget = dock;
  dockWidget.view = view;
  dockWidget.action = action;
  dockWidget.toggle = toggle;

  m_dockWidgets.push_back(dockWidget);
}

void QtMainWindow::overrideView(View* view) {
  const QString name = QString::fromStdString(view->getName());
  if(name == QLatin1String("Tabs")) {
    return;
  }

  QDockWidget* dock = nullptr;
  for(const DockWidget& dockWidget : m_dockWidgets) {
    if(dockWidget.widget->windowTitle() == name) {
      dock = dockWidget.widget;
      break;
    }
  }

  if(dock == nullptr) {
    LOG_ERROR(fmt::format("Couldn't find view to override: {}", name.toStdString()));
    return;
  }

  QWidget* oldWidget = dock->widget()->layout()->itemAt(0)->widget();
  QWidget* newWidget = QtViewWidgetWrapper::getWidgetOfView(view);

  if(oldWidget == newWidget) {
    return;
  }

  oldWidget = dock->widget()->layout()->takeAt(0)->widget();
  oldWidget->hide();
  dock->widget()->layout()->addWidget(newWidget);
  newWidget->show();
}

void QtMainWindow::removeView(View* view) {
  for(size_t i = 0; i < m_dockWidgets.size(); i++) {
    if(m_dockWidgets[i].view == view) {
      removeDockWidget(m_dockWidgets[i].widget);
      m_dockWidgets.erase(m_dockWidgets.begin() + i);
      return;
    }
  }
}

void QtMainWindow::showView(View* view) {
  getDockWidgetForView(view)->widget->setHidden(false);
}

void QtMainWindow::hideView(View* view) {
  getDockWidgetForView(view)->widget->setHidden(true);
}

View* QtMainWindow::findFloatingView(const std::string& name) const {
  for(const auto& m_dockWidget : m_dockWidgets) {
    if(m_dockWidget.view->getName() == name && m_dockWidget.widget->isFloating()) {
      return m_dockWidget.view;
    }
  }

  return nullptr;
}

void QtMainWindow::loadLayout() {
  QSettings settings(QString::fromStdWString(UserPaths::getWindowSettingsFilePath().wstr()), QSettings::IniFormat);

  settings.beginGroup(QStringLiteral("MainWindow"));
  resize(settings.value(QStringLiteral("size"), QSize(600, 400)).toSize());
  move(settings.value(QStringLiteral("position"), QPoint(200, 200)).toPoint());
  if(settings.value(QStringLiteral("maximized"), false).toBool()) {
    showMaximized();
  }
  setShowDockWidgetTitleBars(settings.value(QStringLiteral("showTitleBars"), true).toBool());
  settings.endGroup();
  loadDockWidgetLayout();
}

void QtMainWindow::loadDockWidgetLayout() {
  QSettings settings(QString::fromStdWString(UserPaths::getWindowSettingsFilePath().wstr()), QSettings::IniFormat);
  this->restoreState(settings.value(QStringLiteral("DOCK_LOCATIONS")).toByteArray());

  for(DockWidget dock : m_dockWidgets) {
    dock.action->setChecked(!dock.widget->isHidden());
  }
}

void QtMainWindow::loadWindow(bool showStartWindow) {
  if(showStartWindow) {
    showStartScreen();
  }
}

void QtMainWindow::saveLayout() {
  QSettings settings(QString::fromStdWString(UserPaths::getWindowSettingsFilePath().wstr()), QSettings::IniFormat);

  settings.beginGroup(QStringLiteral("MainWindow"));
  settings.setValue(QStringLiteral("maximized"), isMaximized());
  if(!isMaximized()) {
    settings.setValue(QStringLiteral("size"), size());
    settings.setValue(QStringLiteral("position"), pos());
  }
  settings.setValue(QStringLiteral("showTitleBars"), m_showDockWidgetTitleBars);
  settings.endGroup();

  settings.setValue(QStringLiteral("DOCK_LOCATIONS"), this->saveState());
}

void QtMainWindow::updateHistoryMenu(std::shared_ptr<MessageBase> message) {
  const size_t historyMenuSize = 20;

  if(const auto* base = dynamic_cast<MessageActivateBase*>(message.get()); base != nullptr) {
    std::vector<SearchMatch> matches = base->getSearchMatches();
    if(!matches.empty() && !matches[0].text.empty()) {
      std::vector<std::shared_ptr<MessageBase>> history = {message};
      std::set<SearchMatch> uniqueMatches = {matches[0]};

      for(const auto& historyMessage : m_history) {
        if(const auto* historyBase = dynamic_cast<MessageActivateBase*>(historyMessage.get()); historyBase != nullptr) {
          if(uniqueMatches.insert(historyBase->getSearchMatches()[0]).second) {
            history.push_back(historyMessage);

            if(history.size() >= historyMenuSize) {
              break;
            }
          }
        }
      }

      m_history = history;
    }
  }

  setupHistoryMenu();
}

void QtMainWindow::clearHistoryMenu() {
  m_history.clear();
  setupHistoryMenu();
}

void QtMainWindow::updateBookmarksMenu(const std::vector<std::shared_ptr<Bookmark>>& bookmarks) {
  m_bookmarks = bookmarks;
  setupBookmarksMenu();
}

void QtMainWindow::setContentEnabled(bool enabled) {
  foreach(QAction* action, menuBar()->actions()) {
    action->setEnabled(enabled);
  }

  for(DockWidget& dock : m_dockWidgets) {
    dock.widget->setEnabled(enabled);
  }
}

void QtMainWindow::refreshStyle() {
  setStyleSheet(utility::getStyleSheet(ResourcePaths::getGuiDirectoryPath().concatenate(L"main/main.css")).c_str());

  QFont tooltipFont = QToolTip::font();
  tooltipFont.setPixelSize(IApplicationSettings::getInstanceRaw()->getFontSize());
  QToolTip::setFont(tooltipFont);
}

void QtMainWindow::setWindowsTaskbarProgress(float progress) {
  m_windowsTaskbarButton.setProgress(progress);
}

void QtMainWindow::hideWindowsTaskbarProgress() {
  m_windowsTaskbarButton.hideProgress();
}

void QtMainWindow::showEvent(QShowEvent* /*event*/) {
  m_windowsTaskbarButton.setWindow(this);
}

void QtMainWindow::keyPressEvent(QKeyEvent* event) {
  switch(event->key()) {
  case Qt::Key_Backspace:
    MessageHistoryUndo().dispatch();
    break;

  case Qt::Key_Escape:
    emit hideScreenSearch();
    emit hideIndexingDialog();
    break;

  case Qt::Key_Slash:
  case Qt::Key_Question:
    emit showScreenSearch();
    break;

  case Qt::Key_R:
    if(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) {
      MessageRefreshUI().dispatch();
    }
    break;

  case Qt::Key_F4:
    if(utility::getOsType() == OS_WINDOWS && event->modifiers() & Qt::ControlModifier) {
      closeTab();
    }
    break;

  case Qt::Key_Space:
    PRINT_TRACES();
    break;

  case Qt::Key_Tab:
    MessageFocusView(MessageFocusView::ViewType::TOGGLE).dispatch();
    break;
  }
}

void QtMainWindow::contextMenuEvent(QContextMenuEvent* event) {
  QtContextMenu menu(event, this);
  menu.addUndoActions();
  menu.show();
}

void QtMainWindow::closeEvent(QCloseEvent* /*event*/) {
  MessageWindowClosed().dispatchImmediately();
}

void QtMainWindow::resizeEvent(QResizeEvent* event) {
  m_windowStack.centerSubWindows();
  QMainWindow::resizeEvent(event);
}

bool QtMainWindow::focusNextPrevChild(bool /*next*/) {
  // makes tab key available in key press event
  return false;
}

void QtMainWindow::about() {
  auto* aboutWindow = createWindow<QtAbout>();
  aboutWindow->setupAbout();
}

void QtMainWindow::openSettings() {
  auto* window = createWindow<QtPreferencesWindow>();
  window->setup();
}

void QtMainWindow::showDocumentation() {
  QDesktopServices::openUrl(QUrl(QString::fromStdString("documentation"_g)));
}

void QtMainWindow::showKeyboardShortcuts() {
  auto* keyboardShortcutWindow = createWindow<QtKeyboardShortcuts>();
  keyboardShortcutWindow->setup();
}

void QtMainWindow::showErrorHelpMessage() {
  MessageErrorsHelpMessage(true).dispatch();
}

void QtMainWindow::showChangelog() {
  QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/CoatiSoftware/Sourcetrail/blob/master/CHANGELOG.md")));
}

void QtMainWindow::showBugtracker() {
  QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/CoatiSoftware/Sourcetrail/issues")));
}

void QtMainWindow::showLicenses() {
  auto* licenseWindow = createWindow<QtLicenseWindow>();
  licenseWindow->setup();
}

void QtMainWindow::showDataFolder() {
  QDesktopServices::openUrl(QUrl(
      QString::fromStdWString(L"file:///" + UserPaths::getUserDataDirectoryPath().makeCanonical().wstr()), QUrl::TolerantMode));
}

void QtMainWindow::showLogFolder() {
  QDesktopServices::openUrl(QUrl(
      QString::fromStdWString(L"file:///" + IApplicationSettings::getInstanceRaw()->getLogDirectoryPath().wstring()), QUrl::TolerantMode));
}

void QtMainWindow::openTab() {
  MessageTabOpen().dispatch();
}

void QtMainWindow::closeTab() {
  MessageTabClose().dispatch();
}

void QtMainWindow::nextTab() {
  MessageTabSelect(true).dispatch();
}

void QtMainWindow::previousTab() {
  MessageTabSelect(false).dispatch();
}

void QtMainWindow::showStartScreen() {
  if(dynamic_cast<qt::window::QtStartScreen*>(m_windowStack.getTopWindow()) != nullptr) {
    return;
  }

  auto* pStartScreen = createWindow<qt::window::QtStartScreen>();
  pStartScreen->setupStartScreen();

  connect(pStartScreen, &qt::window::QtStartScreen::openOpenProjectDialog, this, &QtMainWindow::openProject);
  connect(pStartScreen, &qt::window::QtStartScreen::openNewProjectDialog, this, &QtMainWindow::newProject);
}

void QtMainWindow::hideStartScreen() {
  m_windowStack.clearWindows();
}

void QtMainWindow::newProject() {
  auto* wizard = createWindow<QtProjectWizard>();
  wizard->newProject();
}

void QtMainWindow::newProjectFromCDB(const FilePath& filePath) {
  auto* wizard = dynamic_cast<QtProjectWizard*>(m_windowStack.getTopWindow());
  if(wizard == nullptr) {
    wizard = createWindow<QtProjectWizard>();
  }

  wizard->newProjectFromCDB(filePath);
}

void QtMainWindow::openProject() {
  QString fileName = QtFileDialog::getOpenFileName(
      this, tr("Open File"), FilePath(), QStringLiteral("Sourcetrail Project Files (*.srctrlprj)"));

  if(!fileName.isEmpty()) {
    MessageLoadProject(FilePath(fileName.toStdWString())).dispatch();
    m_windowStack.clearWindows();
  }
}

void QtMainWindow::editProject() {
  std::shared_ptr<const Project> currentProject = Application::getInstance()->getCurrentProject();
  if(currentProject) {
    auto* wizard = createWindow<QtProjectWizard>();

    wizard->editProject(currentProject->getProjectSettingsFilePath());
  }
}

void QtMainWindow::closeProject() {
  if(Application::getInstance()->getCurrentProject()) {
    MessageCloseProject().dispatch();
    showStartScreen();
  }
}

void QtMainWindow::find() {
  MessageFind().dispatch();
}

void QtMainWindow::findFulltext() {
  MessageFind(true).dispatch();
}

void QtMainWindow::findOnScreen() {
  emit showScreenSearch();
}

void QtMainWindow::codeReferencePrevious() {
  MessageCodeReference(MessageCodeReference::REFERENCE_PREVIOUS, false).dispatch();
}

void QtMainWindow::codeReferenceNext() {
  MessageCodeReference(MessageCodeReference::REFERENCE_NEXT, false).dispatch();
}

void QtMainWindow::codeLocalReferencePrevious() {
  MessageCodeReference(MessageCodeReference::REFERENCE_PREVIOUS, true).dispatch();
}

void QtMainWindow::codeLocalReferenceNext() {
  MessageCodeReference(MessageCodeReference::REFERENCE_NEXT, true).dispatch();
}

void QtMainWindow::customTrail() {
  MessageCustomTrailShow().dispatch();
}

void QtMainWindow::overview() {
  MessageActivateOverview().dispatch();
}

void QtMainWindow::closeWindow() {
  auto* app = dynamic_cast<QApplication*>(QCoreApplication::instance());

  QWidget* activeWindow = app->activeWindow();
  if(activeWindow != nullptr) {
    activeWindow->close();
  }
}

void QtMainWindow::refresh() {
  MessageIndexingShowDialog().dispatch();
  MessageRefresh().dispatch();
}

void QtMainWindow::forceRefresh() {
  MessageIndexingShowDialog().dispatch();
  MessageRefresh().refreshAll().dispatch();
}

void QtMainWindow::saveAsImage() {
  QString filePath = QtFileDialog::showSaveFileDialog(
      this, tr("Save as Image"), FilePath(), "PNG (*.png);;JPEG (*.JPEG);;BMP Files (*.bmp)");
  if(filePath.isNull()) {
    return;
  }
  MessageSaveAsImage m(filePath);
  m.setSchedulerId(TabId::currentTab());
  m.dispatch();
}

void QtMainWindow::undo() {
  MessageHistoryUndo().dispatch();
}

void QtMainWindow::redo() {
  MessageHistoryRedo().dispatch();
}

void QtMainWindow::zoomIn() {
  MessageZoom(true).dispatch();
}

void QtMainWindow::zoomOut() {
  MessageZoom(false).dispatch();
}

void QtMainWindow::resetZoom() {
  MessageResetZoom().dispatch();
}

void QtMainWindow::resetWindowLayout() {
  FileSystem::remove(UserPaths::getWindowSettingsFilePath());
  FileSystem::copyFile(
      ResourcePaths::getFallbackDirectoryPath().concatenate(L"window_settings.ini"), UserPaths::getWindowSettingsFilePath());
  loadDockWidgetLayout();
}

void QtMainWindow::openRecentProject() {
  auto* action = qobject_cast<QAction*>(sender());
  if(action != nullptr) {
    MessageLoadProject(FilePath(action->data().toString().toStdWString())).dispatch();
    m_windowStack.clearWindows();
  }
}

void QtMainWindow::updateRecentProjectsMenu() {
  m_recentProjectsMenu->clear();

  const std::vector<std::filesystem::path> recentProjects = IApplicationSettings::getInstanceRaw()->getRecentProjects();
  const size_t recentProjectsCount = IApplicationSettings::getInstanceRaw()->getMaxRecentProjectsCount();

  for(size_t i = 0; i < recentProjects.size() && i < recentProjectsCount; ++i) {
    const std::filesystem::path& project = recentProjects[i];
    std::error_code errorCode;
    if(std::filesystem::exists(project, errorCode)) {
      auto* recentProject = new QAction(this);
      recentProject->setText(QString::fromStdWString(project.filename().wstring()));
      recentProject->setData(QString::fromStdWString(project.wstring()));
      connect(recentProject, &QAction::triggered, this, &QtMainWindow::openRecentProject);
      m_recentProjectsMenu->addAction(recentProject);
    }
  }
}

void QtMainWindow::toggleView(View* view, bool fromMenu) {
  DockWidget* dock = getDockWidgetForView(view);

  if(fromMenu) {
    dock->widget->setVisible(dock->action->isChecked());
  } else {
    dock->action->setChecked(dock->widget->isVisible());
  }
}

void QtMainWindow::toggleShowDockWidgetTitleBars() {
  setShowDockWidgetTitleBars(!m_showDockWidgetTitleBars);
}

void QtMainWindow::showBookmarkCreator() {
  MessageBookmarkCreate().dispatch();
}

void QtMainWindow::showBookmarkBrowser() {
  MessageBookmarkBrowse().dispatch();
}

void QtMainWindow::openHistoryAction() {
  auto* action = qobject_cast<QAction*>(sender());
  if(action != nullptr) {
    std::shared_ptr<MessageBase> m = m_history[action->data().toInt()];
    m->setSchedulerId(TabId::currentTab());
    m->setIsReplayed(false);
    m->dispatch();
  }
}

void QtMainWindow::activateBookmarkAction() {
  auto* action = qobject_cast<QAction*>(sender());
  if(action != nullptr) {
    std::shared_ptr<Bookmark> bookmark = m_bookmarks[action->data().toInt()];
    MessageBookmarkActivate(bookmark).dispatch();
  }
}

void QtMainWindow::setupProjectMenu() {
  auto* menu = new QMenu(tr("&Project"), this);
  menuBar()->addMenu(menu);

  menu->addAction(tr("&New Project..."), this, &QtMainWindow::newProject, QKeySequence::New);
  menu->addAction(tr("&Open Project..."), this, &QtMainWindow::openProject, QKeySequence::Open);

  m_recentProjectsMenu = new QMenu(tr("Recent Projects"));    // NOLINT(cppcoreguidelines-owning-memory)
  menu->addMenu(m_recentProjectsMenu);
  updateRecentProjectsMenu();

  menu->addSeparator();

  menu->addAction(tr("&Edit Project..."), this, &QtMainWindow::editProject);
  menu->addSeparator();

  menu->addAction(tr("Close Project"), this, &QtMainWindow::closeProject);
  menu->addAction(tr("E&xit"), QCoreApplication::instance(), &QCoreApplication::quit, QKeySequence::Quit);
}

void QtMainWindow::setupEditMenu() {
  auto* menu = new QMenu(tr("&Edit"), this);    // NOLINT(cppcoreguidelines-owning-memory)
  menuBar()->addMenu(menu);

  menu->addAction(tr("&Refresh"), this, &QtMainWindow::refresh, QKeySequence::Refresh);
  if(utility::getOsType() == OS_WINDOWS) {
    menu->addAction(tr("&Full Refresh"), this, &QtMainWindow::forceRefresh, QKeySequence(tr("Shift+F5")));
  } else {
    menu->addAction(tr("&Full Refresh"), this, &QtMainWindow::forceRefresh, QKeySequence(tr("Shift+Ctrl+R")));
  }

  menu->addSeparator();

  menu->addAction(tr("&Find Symbol"), this, &QtMainWindow::find, QKeySequence::Find);
  menu->addAction(tr("&Find Text"), this, &QtMainWindow::findFulltext, QKeySequence(tr("Shift+Ctrl+F")));
  menu->addAction(tr("&Find On-Screen"), this, &QtMainWindow::findOnScreen, QKeySequence(tr("Ctrl+D")));

  menu->addSeparator();

  menu->addAction(tr("Next Reference"), this, &QtMainWindow::codeReferenceNext, QKeySequence("Ctrl+G"));
  menu->addAction(tr("Previous Reference"), this, &QtMainWindow::codeReferencePrevious, QKeySequence("Shift+Ctrl+G"));

  menu->addAction(tr("Next Local Reference"), this, &QtMainWindow::codeLocalReferenceNext, QKeySequence("Ctrl+L"));
  menu->addAction(tr("Previous Local Reference"),
                  this,
                  &QtMainWindow::codeLocalReferencePrevious,
                  QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_L));

  menu->addSeparator();

  menu->addAction(tr("Custom Trail..."), this, &QtMainWindow::customTrail, QKeySequence("Ctrl+U"));

  menu->addSeparator();

  menu->addAction(tr("&To overview"), this, &QtMainWindow::overview, QKeySequence(Qt::CTRL | Qt::Key_Home));

  menu->addSeparator();

  menu->addAction(tr("&Save Graph as Image..."), this, &QtMainWindow::saveAsImage, QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_S));

  menu->addSeparator();

  menu->addAction(tr("Preferences..."), this, &QtMainWindow::openSettings, QKeySequence(Qt::CTRL | Qt::Key_Comma));
}

void QtMainWindow::setupViewMenu() {
  auto* menu = new QMenu(tr("&View"), this);    // NOLINT(cppcoreguidelines-owning-memory)
  menuBar()->addMenu(menu);

  menu->addAction(tr("New Tab"), this, &QtMainWindow::openTab, QKeySequence::AddTab);
  menu->addAction(tr("Close Tab"), this, &QtMainWindow::closeTab, QKeySequence("Ctrl+W"));

  if(utility::getOsType() == OS_MAC) {
    menu->addAction(tr("Select Next Tab"), this, &QtMainWindow::nextTab, QKeySequence(Qt::META | Qt::Key_Tab));
    menu->addAction(tr("Select Previous Tab"), this, &QtMainWindow::previousTab, QKeySequence(Qt::SHIFT | Qt::META | Qt::Key_Tab));
  } else {
    menu->addAction(tr("Select Next Tab"), this, &QtMainWindow::nextTab, QKeySequence(Qt::CTRL | Qt::Key_Tab));
    menu->addAction(tr("Select Previous Tab"), this, &QtMainWindow::previousTab, QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_Tab));
  }

  menu->addSeparator();

  menu->addAction(tr("Show Start Window"), this, &QtMainWindow::showStartScreen);

  m_showTitleBarsAction = new QAction(QStringLiteral("Show Title Bars"), this);    // NOLINT(cppcoreguidelines-owning-memory)
  m_showTitleBarsAction->setCheckable(true);
  m_showTitleBarsAction->setChecked(m_showDockWidgetTitleBars);
  connect(m_showTitleBarsAction, &QAction::triggered, this, &QtMainWindow::toggleShowDockWidgetTitleBars);
  menu->addAction(m_showTitleBarsAction);
  menu->addAction(tr("Reset Window Layout"), this, &QtMainWindow::resetWindowLayout);

  menu->addSeparator();

  m_viewSeparator = menu->addSeparator();

  menu->addAction(tr("Larger Font"), this, &QtMainWindow::zoomIn, QKeySequence::ZoomIn);
  menu->addAction(tr("Smaller Font"), this, &QtMainWindow::zoomOut, QKeySequence::ZoomOut);
  menu->addAction(tr("Reset Font Size"), this, &QtMainWindow::resetZoom, QKeySequence("Ctrl+0"));

  m_viewMenu = menu;
}

void QtMainWindow::setupHistoryMenu() {
  if(m_historyMenu == nullptr) {
    m_historyMenu = new QMenu(tr("&History"), this);    // NOLINT(cppcoreguidelines-owning-memory)
    menuBar()->addMenu(m_historyMenu);
  } else {
    m_historyMenu->clear();
  }

  m_historyMenu->addAction(tr("Back"), this, &QtMainWindow::undo, QKeySequence::Back);
  m_historyMenu->addAction(tr("Forward"), this, &QtMainWindow::redo, QKeySequence::Forward);

  m_historyMenu->addSeparator();

  auto* title = new QAction(tr("Recently Active Symbols"));
  title->setEnabled(false);
  m_historyMenu->addAction(title);

  for(size_t i = 0; i < m_history.size(); i++) {
    auto* msg = dynamic_cast<MessageActivateBase*>(m_history[i].get());
    if(msg == nullptr) {
      continue;
    }

    const SearchMatch match = msg->getSearchMatches()[0];
    const std::wstring name = utility::elide(match.getFullName(), utility::ElideMode::RIGHT, 50);

    auto* action = new QAction;    // NOLINT(cppcoreguidelines-owning-memory)
    action->setText(QString::fromStdWString(name));
    action->setData(QVariant(int(i)));

    connect(action, &QAction::triggered, this, &QtMainWindow::openHistoryAction);
    m_historyMenu->addAction(action);
  }
}

void QtMainWindow::setupBookmarksMenu() {
  if(m_bookmarksMenu == nullptr) {
    m_bookmarksMenu = new QMenu(tr("&Bookmarks"), this);    // NOLINT(cppcoreguidelines-owning-memory)
    menuBar()->addMenu(m_bookmarksMenu);
  } else {
    m_bookmarksMenu->clear();
  }

  m_bookmarksMenu->addAction(tr("Bookmark Active Symbol..."), this, &QtMainWindow::showBookmarkCreator, QKeySequence::Save);
  m_bookmarksMenu->addAction(tr("Bookmark Manager"), this, &QtMainWindow::showBookmarkBrowser, QKeySequence("Ctrl+B"));

  m_bookmarksMenu->addSeparator();

  auto* title = new QAction(tr("Recent Bookmarks"));    // NOLINT(cppcoreguidelines-owning-memory)
  title->setEnabled(false);
  m_bookmarksMenu->addAction(title);

  for(size_t i = 0; i < m_bookmarks.size(); i++) {
    Bookmark* bookmark = m_bookmarks[i].get();
    std::wstring name = utility::elide(bookmark->getName(), utility::ElideMode::RIGHT, 50);

    auto* action = new QAction;    // NOLINT(cppcoreguidelines-owning-memory)
    action->setText(QString::fromStdWString(name));
    action->setData(QVariant(int(i)));

    connect(action, &QAction::triggered, this, &QtMainWindow::activateBookmarkAction);
    m_bookmarksMenu->addAction(action);
  }
}

void QtMainWindow::setupHelpMenu() {
  auto* menu = new QMenu(tr("&Help"), this);    // NOLINT(cppcoreguidelines-owning-memory)
  menuBar()->addMenu(menu);

  menu->addAction(tr("Keyboard Shortcuts"), this, &QtMainWindow::showKeyboardShortcuts);
  menu->addAction(tr("Fixing Errors"), this, &QtMainWindow::showErrorHelpMessage);
  menu->addAction(tr("Documentation"), this, &QtMainWindow::showDocumentation);
  menu->addAction(tr("Changelog"), this, &QtMainWindow::showChangelog);
  menu->addAction(tr("Bug Tracker"), this, &QtMainWindow::showBugtracker);

  menu->addSeparator();

  menu->addAction(tr("License"), this, &QtMainWindow::showLicenses);
  menu->addAction(tr("&About Sourcetrail"), this, &QtMainWindow::about);

  menu->addSeparator();

  menu->addAction(tr("Show Data Folder"), this, &QtMainWindow::showDataFolder);
  menu->addAction(tr("Show Log Folder"), this, &QtMainWindow::showLogFolder);
}

QtMainWindow::DockWidget* QtMainWindow::getDockWidgetForView(View* view) {
  for(DockWidget& dock : m_dockWidgets) {
    if(dock.view == view) {
      return &dock;
    }

    if(const auto* compositeView = dynamic_cast<const CompositeView*>(dock.view); compositeView != nullptr) {
      for(const View* compositeV : compositeView->getViews()) {
        if(compositeV == view) {
          return &dock;
        }
      }
    }

    if(const auto* tabbedView = dynamic_cast<const TabbedView*>(dock.view); tabbedView != nullptr) {
      for(const View* tabbedV : tabbedView->getViews()) {
        if(tabbedV == view) {
          return &dock;
        }
      }
    }
  }

  LOG_ERROR("DockWidget was not found for view.");
  return nullptr;
}

void QtMainWindow::setShowDockWidgetTitleBars(bool showTitleBars) {
  m_showDockWidgetTitleBars = showTitleBars;

  if(m_showTitleBarsAction != nullptr) {
    m_showTitleBarsAction->setChecked(showTitleBars);
  }

  for(DockWidget& dock : m_dockWidgets) {
    if(showTitleBars) {
      dock.widget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
      dock.widget->setTitleBarWidget(nullptr);
    } else {
      dock.widget->setFeatures(QDockWidget::NoDockWidgetFeatures);
      dock.widget->setTitleBarWidget(new QWidget);    // NOLINT(cppcoreguidelines-owning-memory)
    }
  }
}

template <typename T>
T* QtMainWindow::createWindow() {
  T* pWindow = new T(this);    // NOLINT(cppcoreguidelines-owning-memory)

  connect(pWindow, &QtWindow::canceled, &m_windowStack, &QtWindowStack::popWindow);
  connect(pWindow, &QtWindow::finished, &m_windowStack, &QtWindowStack::clearWindows);

  m_windowStack.pushWindow(pWindow);

  return pWindow;
}