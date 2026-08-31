#pragma once
#include <memory>

#include <QObject>
#include <QStringList>

#include <qqmlintegration.h>

#include "shell/QmlStatusView.h"
#include "shell/ShellSnapshot.h"

class Component;
class QJSEngine;
class QQmlEngine;
class StatusBarController;
class StatusController;
class StorageAccess;

namespace shell {

class QmlStatusBarView;
class QmlViewLayout;

/**
 * The status bar and the status log behind it.
 *
 * Two controllers, one view-model, because the design shows them as one thing: the bar along the
 * bottom, and the scrollback the user opens from it. Both are driven from the message bus --
 * MessageStatus, MessageIndexingStatus, MessageErrorCountUpdate, MessagePingReceived -- and
 * neither needs anything from the GUI, so this is a pure read-out.
 *
 * Index progress arrives here as well as through AppShell's dialog view; this is the one the
 * status bar binds to, because it is the one the controller keeps in step with the error count
 * and the IDE connection.
 */
class StatusViewModel final : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QString message READ message NOTIFY statusChanged)
  Q_PROPERTY(bool isError READ isError NOTIFY statusChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY statusChanged)
  Q_PROPERTY(QString ideStatus READ ideStatus NOTIFY statusChanged)
  Q_PROPERTY(int errorCount READ errorCount NOTIFY statusChanged)
  Q_PROPERTY(int fatalCount READ fatalCount NOTIFY statusChanged)

  /** -1 when no index run is in progress, so QML can tell that from "0% done". */
  Q_PROPERTY(int indexingPercent READ indexingPercent NOTIFY statusChanged)
  Q_PROPERTY(bool indexing READ indexing NOTIFY statusChanged)

  /** The status scrollback, newest last. */
  Q_PROPERTY(QStringList log READ log NOTIFY logChanged)

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
  explicit StatusViewModel(QObject* parent);
  ~StatusViewModel() override;

  static StatusViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

  /** Called by AppShell::setup(), once the message queue exists. */
  void attach(StorageAccess* storageAccess);

  [[nodiscard]] QString message() const;
  [[nodiscard]] bool isError() const;
  [[nodiscard]] bool busy() const;
  [[nodiscard]] QString ideStatus() const;
  [[nodiscard]] int errorCount() const;
  [[nodiscard]] int fatalCount() const;
  [[nodiscard]] int indexingPercent() const;
  [[nodiscard]] bool indexing() const;
  [[nodiscard]] QStringList log() const;

Q_SIGNALS:
  void statusChanged();
  void logChanged();
  /** A controller asked for its panel to be brought forward; the frame decides how. */
  void viewRaiseRequested(const QString& viewName);

private:
  void applyStatus(const StatusSnapshot& snapshot);
  void applyLog(const QList<StatusLine>& lines);

  static StatusViewModel* sInstance;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  StatusSnapshot mStatus;
  QStringList mLog;

  std::unique_ptr<QmlViewLayout> mLayout;

  std::shared_ptr<QmlStatusBarView> mBarView;
  std::shared_ptr<StatusBarController> mBarController;
  std::unique_ptr<Component> mBarComponent;

  std::shared_ptr<QmlStatusView> mLogView;
  std::shared_ptr<StatusController> mLogController;
  std::unique_ptr<Component> mLogComponent;
};

}    // namespace shell
