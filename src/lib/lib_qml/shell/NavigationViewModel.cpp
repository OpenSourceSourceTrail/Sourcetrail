#include "shell/NavigationViewModel.h"

#include <utility>

#include <QJSEngine>
#include <QQmlEngine>

#include "component/Component.h"
#include "component/controller/UndoRedoController.h"
#include "GuiThread.h"
#include "logging.h"
#include "MessageQueue.h"
#include "shell/QmlUndoRedoView.h"
#include "type/history/MessageHistoryRedo.h"
#include "type/history/MessageHistoryToPosition.h"
#include "type/history/MessageHistoryUndo.h"

namespace shell {

NavigationViewModel* NavigationViewModel::sInstance = nullptr;

NavigationViewModel::NavigationViewModel(QObject* parent) : QObject(parent) {
  sInstance = this;
}

NavigationViewModel::~NavigationViewModel() {
  sInstance = nullptr;
}

NavigationViewModel* NavigationViewModel::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
  // AppShell owns the view-model for the whole run; the QML engine must not delete it.
  QJSEngine::setObjectOwnership(sInstance, QJSEngine::CppOwnership);
  return sInstance;
}

void NavigationViewModel::attach(StorageAccess* storageAccess) {
  if(mComponent) {
    return;
  }
  // UndoRedoController registers with IMessageQueue in its constructor, so the queue has to exist
  // first. AppShell::setup() is called from inside Application::createInstance, after it is set.
  if(!IMessageQueue::getInstance()) {
    LOG_ERROR("Cannot build the navigation component before the message queue exists.");
    return;
  }

  mView = std::make_shared<QmlUndoRedoView>([this](HistorySnapshot snapshot) {
    // Called on the message-bus thread. Hand the whole thing over by value and return.
    qml::postToGui(this, [this, snapshot = std::move(snapshot)]() { applySnapshot(snapshot); });
  });
  mController = std::make_shared<UndoRedoController>(storageAccess);
  mComponent = std::make_unique<Component>(mView, mController);
}

bool NavigationViewModel::canGoBack() const {
  return mCanGoBack;
}

bool NavigationViewModel::canGoForward() const {
  return mCanGoForward;
}

QAbstractItemModel* NavigationViewModel::history() {
  return &mHistory;
}

int NavigationViewModel::currentIndex() const {
  return mCurrentIndex;
}

void NavigationViewModel::goBack() {
  MessageHistoryUndo{}.dispatch();
}

void NavigationViewModel::goForward() {
  MessageHistoryRedo{}.dispatch();
}

void NavigationViewModel::goToPosition(int index) {
  if(index < 0) {
    return;
  }
  MessageHistoryToPosition{static_cast<size_t>(index)}.dispatch();
}

void NavigationViewModel::applySnapshot(const HistorySnapshot& snapshot) {
  mHistory.setItems(snapshot.items);
  mCanGoBack = snapshot.canUndo;
  mCanGoForward = snapshot.canRedo;
  mCurrentIndex = snapshot.currentIndex;
  Q_EMIT historyChanged();
}

}    // namespace shell
