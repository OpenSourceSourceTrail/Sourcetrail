#pragma once
#include <memory>

#include <QObject>

#include <qqmlintegration.h>

#include "shell/HistoryModel.h"

class Component;
class QJSEngine;
class QQmlEngine;
class StorageAccess;
class UndoRedoController;

namespace shell {

class QmlUndoRedoView;

/**
 * The toolbar's back, forward and history controls.
 *
 * UndoRedoController already holds the navigation stack and listens to every message that counts
 * as a step -- activating a symbol, expanding a node, showing an error, scrolling code. This does
 * not re-derive any of that; it turns what the controller reports into two booleans and a list
 * model, and turns clicks back into the three history messages.
 */
class NavigationViewModel final : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
  Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
  Q_PROPERTY(QAbstractItemModel* history READ history CONSTANT)
  Q_PROPERTY(int currentIndex READ currentIndex NOTIFY historyChanged)

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
  explicit NavigationViewModel(QObject* parent);
  ~NavigationViewModel() override;

  static NavigationViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

  /** Called by AppShell::setup(), once the message queue exists. */
  void attach(StorageAccess* storageAccess);

  [[nodiscard]] bool canGoBack() const;
  [[nodiscard]] bool canGoForward() const;
  [[nodiscard]] QAbstractItemModel* history();
  [[nodiscard]] int currentIndex() const;

  /** @name What QML does. All dispatch a message and return. @{ */
  Q_INVOKABLE void goBack();
  Q_INVOKABLE void goForward();
  Q_INVOKABLE void goToPosition(int index);
  /** @} */

Q_SIGNALS:
  void historyChanged();

private:
  void applySnapshot(const HistorySnapshot& snapshot);

  static NavigationViewModel* sInstance;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  HistoryModel mHistory;

  std::shared_ptr<QmlUndoRedoView> mView;
  std::shared_ptr<UndoRedoController> mController;
  std::unique_ptr<Component> mComponent;

  bool mCanGoBack = false;
  bool mCanGoForward = false;
  int mCurrentIndex = -1;
};

}    // namespace shell
