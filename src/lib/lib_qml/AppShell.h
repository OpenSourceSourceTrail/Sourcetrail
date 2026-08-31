#pragma once
#include <memory>
#include <vector>

#include <QObject>
#include <QString>
#include <QUrl>

#include <qqmlintegration.h>

#include "app/IAppShell.hpp"

class QQmlEngine;
class QJSEngine;
class ShellMessages;
class ActivationController;

namespace graph {
class GraphViewModel;
}

namespace search {
class SearchViewModel;
}

namespace shell {
class NavigationViewModel;
class StatusViewModel;
}    // namespace shell

/**
 * The QML front end's implementation of everything Application asks of a user interface.
 *
 * There is deliberately no View/ViewFactory/ComponentManager underneath: QML binds straight to the
 * properties here and to the view-models the shell owns. Application calls into this from the
 * message bus thread, so every method either only touches atomics or hops to the GUI thread
 * through qml::postToGui -- see GuiThread.h for why that hop never blocks the caller.
 *
 * The instance is created by main() and lives for the whole run; QML reaches it as the singleton
 * `AppShell`, which is why create() hands out a pointer it does not own.
 */
class AppShell final
    : public QObject
    , public lib::IAppShell {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QString title READ title NOTIFY titleChanged)
  Q_PROPERTY(QString status READ status NOTIFY statusChanged)
  Q_PROPERTY(bool statusIsError READ statusIsError NOTIFY statusChanged)
  Q_PROPERTY(bool projectLoaded READ projectLoaded NOTIFY projectLoadedChanged)
  Q_PROPERTY(bool indexing READ indexing NOTIFY progressChanged)
  Q_PROPERTY(QString progressMessage READ progressMessage NOTIFY progressChanged)
  Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressChanged)
  Q_PROPERTY(QStringList recentProjects READ recentProjects NOTIFY recentProjectsChanged)
  Q_PROPERTY(QString projectSummary READ projectSummary NOTIFY projectSummaryChanged)
  Q_PROPERTY(int uiFontSize READ uiFontSize WRITE setUiFontSize NOTIFY uiFontSizeChanged)

public:
  /**
   * Takes a parent explicitly, with no default, so the type is NOT default-constructible.
   *
   * That is load-bearing, not style. QQmlPrivate::singletonConstructionMode() picks
   * SingletonConstructionMode::Constructor whenever std::is_default_constructible is true, and only
   * falls through to the create() factory when it is false -- so a default-constructible
   * QML_SINGLETON silently gets a *second*, engine-owned instance and create() is never called.
   * QML then binds to that orphan while C++ updates the real one.
   */
  explicit AppShell(QObject* parent);
  ~AppShell() override;

  /** QML singleton accessor. Ownership stays with main(). */
  static AppShell* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

  [[nodiscard]] QString title() const;
  [[nodiscard]] QString status() const;
  [[nodiscard]] bool statusIsError() const;
  [[nodiscard]] bool projectLoaded() const;
  [[nodiscard]] bool indexing() const;
  [[nodiscard]] QString progressMessage() const;
  [[nodiscard]] int progressPercent() const;
  [[nodiscard]] QStringList recentProjects() const;
  [[nodiscard]] QString projectSummary() const;

  /** Interface font size in points. Theme.qml scales its whole type ramp from this. */
  [[nodiscard]] int uiFontSize() const;
  void setUiFontSize(int pointSize);

  /** @name Actions QML triggers. All of these dispatch and return; none of them block. @{ */
  Q_INVOKABLE void loadProject(const QUrl& projectFile);
  Q_INVOKABLE void refresh(bool all);
  Q_INVOKABLE void cancelIndexing();
  Q_INVOKABLE void quit();
  /** @} */

  /** @name lib::IAppShell -- called from the message bus thread. @{ */
  void setup() override;
  void saveLayout() override;
  void clear() override;
  std::shared_ptr<DialogView> getDialogView(DialogView::UseCase useCase) override;
  void refreshViews() override;
  void refreshUIState(bool isAfterIndexing) override;
  void loadWindow(bool showStartWindow) override;
  void hideStartScreen() override;
  void activateWindow() override;
  void setTitle(const std::wstring& title) override;
  void updateRecentProjectMenu() override;
  void updateHistoryMenu(std::shared_ptr<MessageBase> message) override;
  void updateBookmarksMenu(const std::vector<std::shared_ptr<Bookmark>>& bookmarks) override;
  /** @} */

  /** @name Called by QmlDialogView, from whichever thread is indexing. @{ */
  void reportProgress(const QString& message, int percent);
  void clearProgress();
  /** @} */

  /** Called by the message listeners the shell owns. */
  void reportStatus(const QString& status, bool isError);

Q_SIGNALS:
  void titleChanged();
  void statusChanged();
  void projectLoadedChanged();
  void progressChanged();
  void recentProjectsChanged();
  void projectSummaryChanged();
  void uiFontSizeChanged();
  void windowActivationRequested();
  void startScreenRequested(bool show);
  void viewsNeedRefresh(bool isAfterIndexing);

private:
  static AppShell* sInstance;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  void setProjectLoaded(bool loaded);

  QString mTitle;
  QString mStatus;
  bool mStatusIsError = false;
  bool mProjectLoaded = false;
  bool mIndexing = false;
  QString mProgressMessage;
  int mProgressPercent = 0;
  QStringList mRecentProjects;
  QString mProjectSummary;

  std::unique_ptr<ShellMessages> mMessages;

  // The activation controller has no view of its own: it turns "activate this node" into the
  // MessageActivateTokens every panel actually listens for. Without one the graph never rebuilds.
  std::unique_ptr<ActivationController> mActivation;
  std::unique_ptr<graph::GraphViewModel> mGraph;
  std::unique_ptr<shell::NavigationViewModel> mNavigation;
  std::unique_ptr<shell::StatusViewModel> mStatusModel;
  std::unique_ptr<search::SearchViewModel> mSearch;
};
