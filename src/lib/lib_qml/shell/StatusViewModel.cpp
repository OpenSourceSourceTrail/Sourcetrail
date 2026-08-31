#include "shell/StatusViewModel.h"

#include <utility>

#include <QJSEngine>
#include <QQmlEngine>

#include "component/Component.h"
#include "component/controller/StatusBarController.h"
#include "component/controller/StatusController.h"
#include "GuiThread.h"
#include "logging.h"
#include "MessageQueue.h"
#include "shell/QmlStatusBarView.h"
#include "shell/QmlViewLayout.h"

namespace shell {

StatusViewModel* StatusViewModel::sInstance = nullptr;

StatusViewModel::StatusViewModel(QObject* parent) : QObject(parent) {
  sInstance = this;
}

StatusViewModel::~StatusViewModel() = default;

StatusViewModel* StatusViewModel::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
  // AppShell owns the view-model for the whole run; the QML engine must not delete it.
  QJSEngine::setObjectOwnership(sInstance, QJSEngine::CppOwnership);
  return sInstance;
}

void StatusViewModel::attach(StorageAccess* storageAccess) {
  if(mBarComponent) {
    return;
  }
  // Both controllers register with IMessageQueue in their constructors, so the queue has to exist
  // first. AppShell::setup() is called from inside Application::createInstance, after it is set.
  if(!IMessageQueue::getInstance()) {
    LOG_ERROR("Cannot build the status components before the message queue exists.");
    return;
  }

  // StatusController raises its own panel through View::showView(), which goes through the layout
  // -- so this one cannot be null the way the graph's is.
  mLayout = std::make_unique<QmlViewLayout>([this](const std::string& viewName) {
    const QString name = QString::fromStdString(viewName);
    qml::postToGui(this, [this, name]() { Q_EMIT viewRaiseRequested(name); });
  });

  mBarView = std::make_shared<QmlStatusBarView>(mLayout.get(), [this](StatusSnapshot snapshot) {
    // Called on the message-bus thread. Hand the whole thing over by value and return.
    qml::postToGui(this, [this, snapshot = std::move(snapshot)]() { applyStatus(snapshot); });
  });
  mBarController = std::make_shared<StatusBarController>(storageAccess);
  mBarComponent = std::make_unique<Component>(mBarView, mBarController);

  mLogView = std::make_shared<QmlStatusView>(mLayout.get(), [this](QList<StatusLine> lines) {
    qml::postToGui(this, [this, lines = std::move(lines)]() { applyLog(lines); });
  });
  mLogController = std::make_shared<StatusController>();
  mLogComponent = std::make_unique<Component>(mLogView, mLogController);
}

QString StatusViewModel::message() const {
  return mStatus.message;
}

bool StatusViewModel::isError() const {
  return mStatus.isError;
}

bool StatusViewModel::busy() const {
  return mStatus.showLoader;
}

QString StatusViewModel::ideStatus() const {
  return mStatus.ideStatus;
}

int StatusViewModel::errorCount() const {
  return mStatus.errorTotal;
}

int StatusViewModel::fatalCount() const {
  return mStatus.errorFatal;
}

int StatusViewModel::indexingPercent() const {
  return mStatus.indexingPercent;
}

bool StatusViewModel::indexing() const {
  return mStatus.indexingPercent >= 0;
}

QStringList StatusViewModel::log() const {
  return mLog;
}

void StatusViewModel::applyStatus(const StatusSnapshot& snapshot) {
  mStatus = snapshot;
  Q_EMIT statusChanged();
}

void StatusViewModel::applyLog(const QList<StatusLine>& lines) {
  mLog.clear();
  mLog.reserve(lines.size());
  for(const StatusLine& line : lines) {
    mLog.append(line.message);
  }
  Q_EMIT logChanged();
}

}    // namespace shell
