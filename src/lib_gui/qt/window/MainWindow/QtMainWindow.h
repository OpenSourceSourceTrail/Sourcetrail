#pragma once
#include <memory>
#include <vector>

#include <QMainWindow>

#include <qtclasshelpermacros.h>

#include "QtWindowStack.h"


class Bookmark;
class MessageBase;
class QDockWidget;
class View;
class QtViewToggle;
class FilePath;

class QtMainWindow final : public QMainWindow {
  Q_OBJECT

public:
  QtMainWindow();

  Q_DISABLE_COPY_MOVE(QtMainWindow)

  ~QtMainWindow() override;

  void addView(View* view);
  void overrideView(View* view);
  void removeView(View* view);

  void showView(View* view);
  void hideView(View* view);

  [[nodiscard]] View* findFloatingView(const std::string& name) const;

  void loadLayout();
  void saveLayout();

  void loadDockWidgetLayout();
  void loadWindow(bool showStartWindow);

  void updateHistoryMenu(std::shared_ptr<MessageBase> message);
  void clearHistoryMenu();

  void updateBookmarksMenu(const std::vector<std::shared_ptr<Bookmark>>& bookmarks);

  void setContentEnabled(bool enabled);
  void refreshStyle();

  void setWindowsTaskbarProgress(float progress);
  void hideWindowsTaskbarProgress();

signals:
  void showScreenSearch();
  void hideScreenSearch();
  void hideIndexingDialog();

protected:
  void showEvent(QShowEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

  bool focusNextPrevChild(bool next) override;

public slots:
  void about();
  void openSettings();
  void showChangelog();
  void showBugtracker();
  void showDocumentation();
  void showKeyboardShortcuts();
  void showErrorHelpMessage();
  void showLicenses();

  void showDataFolder();
  void showLogFolder();

  void openTab();
  void closeTab();
  void nextTab();
  void previousTab();

  void showStartScreen();
  void hideStartScreen();

  void newProject();
  void newProjectFromCDB(const FilePath& filePath);
  void openProject();
  void editProject();
  void closeProject();

  void find();
  void findFulltext();
  void findOnScreen();
  void codeReferencePrevious();
  void codeReferenceNext();
  void codeLocalReferencePrevious();
  void codeLocalReferenceNext();
  void customTrail();
  void overview();

  void closeWindow();
  void refresh();
  void forceRefresh();

  void undo();
  void redo();
  void zoomIn();
  void zoomOut();
  void resetZoom();

  void resetWindowLayout();

  void openRecentProject();
  void updateRecentProjectsMenu();

  void toggleView(View* view, bool fromMenu);
  void saveAsImage();

private slots:
  void toggleShowDockWidgetTitleBars();

  void showBookmarkCreator();
  void showBookmarkBrowser();

  void openHistoryAction();
  void activateBookmarkAction();

private:
  struct DockWidget final {
    QDockWidget* widget = nullptr;
    View* view = nullptr;
    QAction* action = nullptr;
    QtViewToggle* toggle = nullptr;
  };

  void setupEditMenu();
  void setupProjectMenu();
  void setupViewMenu();
  void setupHistoryMenu();
  void setupBookmarksMenu();
  void setupHelpMenu();

  DockWidget* getDockWidgetForView(View* view);

  void setShowDockWidgetTitleBars(bool showTitleBars);

  template <typename T>
  T* createWindow();

  std::vector<DockWidget> m_dockWidgets;

  QMenu* m_viewMenu = nullptr;
  QAction* m_viewSeparator = nullptr;

  QMenu* m_historyMenu = nullptr;
  std::vector<std::shared_ptr<MessageBase>> m_history;

  QMenu* m_bookmarksMenu = nullptr;
  std::vector<std::shared_ptr<Bookmark>> m_bookmarks;

  QMenu* m_recentProjectsMenu = nullptr;

  QAction* m_showTitleBarsAction = nullptr;

  bool m_showDockWidgetTitleBars = true;

  QtWindowStack m_windowStack;
};
