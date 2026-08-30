#include "graph/GraphViewModel.h"

#include <utility>

#include <QJSEngine>
#include <QQmlEngine>

#include "component/Component.h"
#include "component/controller/GraphController.h"
#include "graph/QmlGraphView.h"
#include "GuiThread.h"
#include "logging.h"
#include "MessageQueue.h"
#include "type/activation/MessageActivateOverview.h"
#include "type/graph/MessageActivateEdge.h"
#include "type/graph/MessageActivateNodes.h"
#include "type/graph/MessageDeactivateEdge.h"
#include "type/graph/MessageGraphNodeBundleSplit.h"
#include "type/graph/MessageGraphNodeExpand.h"
#include "type/graph/MessageGraphNodeHide.h"
#include "type/tab/MessageTabOpenWith.h"

namespace graph {

namespace {

GroupType groupingFromInt(int value) {
  switch(value) {
  case 1:
    return GroupType::FILE_TYPE;
  case 2:
    return GroupType::NAMESPACE;
  default:
    return GroupType::NONE;
  }
}

}    // namespace

GraphViewModel* GraphViewModel::sInstance = nullptr;

GraphViewModel::GraphViewModel() {
  sInstance = this;
}

GraphViewModel::~GraphViewModel() {
  sInstance = nullptr;
}

GraphViewModel* GraphViewModel::create(QQmlEngine* /*qmlEngine*/, QJSEngine* /*jsEngine*/) {
  // AppShell owns the view-model for the whole run; the QML engine must not delete it.
  QJSEngine::setObjectOwnership(sInstance, QJSEngine::CppOwnership);
  return sInstance;
}

void GraphViewModel::attach(StorageAccess* storageAccess) {
  if(mComponent) {
    return;
  }
  // GraphController registers with IMessageQueue in its constructor, so the queue has to exist
  // first. AppShell::setup() is called from inside Application::createInstance, after it is set.
  if(!IMessageQueue::getInstance()) {
    LOG_ERROR("Cannot build the graph component before the message queue exists.");
    return;
  }

  mView = std::make_shared<QmlGraphView>([this](Snapshot snapshot) {
    // Called on the message-bus thread. Hand the whole thing over by value and return.
    qml::postToGui(this, [this, snapshot = std::move(snapshot)]() mutable { applySnapshot(std::move(snapshot)); });
  });
  mController = std::make_shared<GraphController>(storageAccess);
  mComponent = std::make_unique<Component>(mView, mController);
}

void GraphViewModel::detach() {
  mComponent.reset();
  mController.reset();
  mView.reset();
}

QAbstractItemModel* GraphViewModel::nodes() {
  return &mNodes;
}

QAbstractItemModel* GraphViewModel::edges() {
  return &mEdges;
}

qreal GraphViewModel::boundsX() const {
  return mBoundsX;
}

qreal GraphViewModel::boundsY() const {
  return mBoundsY;
}

qreal GraphViewModel::boundsWidth() const {
  return mBoundsWidth;
}

qreal GraphViewModel::boundsHeight() const {
  return mBoundsHeight;
}

bool GraphViewModel::empty() const {
  return mNodes.rowCount() == 0;
}

int GraphViewModel::grouping() const {
  return mGrouping;
}

void GraphViewModel::setGrouping(int grouping) {
  if(mGrouping == grouping) {
    return;
  }
  mGrouping = grouping;
  if(mView) {
    mView->setLayoutInputs({static_cast<float>(mViewportWidth), static_cast<float>(mViewportHeight)}, groupingFromInt(mGrouping));
  }
  Q_EMIT groupingChanged();

  // Grouping is a layout input, so the graph has to be laid out again to take it.
  MessageActivateOverview{}.dispatch();
}

void GraphViewModel::setViewportSize(qreal width, qreal height) {
  if(width <= 0 || height <= 0) {
    return;
  }
  mViewportWidth = width;
  mViewportHeight = height;
  if(mView) {
    mView->setLayoutInputs({static_cast<float>(width), static_cast<float>(height)}, groupingFromInt(mGrouping));
  }
}

void GraphViewModel::activateNode(qulonglong tokenId, bool isActive, bool multipleActive) {
  // Clicking the one active node clears the edge selection instead of re-activating it.
  if(isActive && !multipleActive) {
    MessageDeactivateEdge{true}.dispatch();
    return;
  }
  MessageActivateNodes{static_cast<Id>(tokenId)}.dispatch();
}

void GraphViewModel::activateEdge(qulonglong edgeId) {
  const auto found = mEdgeActivations.find(static_cast<Id>(edgeId));
  if(found == mEdgeActivations.end()) {
    return;
  }
  const EdgeActivation& activation = found->second;

  if(activation.expandable) {
    MessageGraphNodeBundleSplit{activation.expandTokenId}.dispatch();
    return;
  }

  MessageActivateEdge message{static_cast<Id>(edgeId), activation.type, activation.sourceName, activation.targetName};
  message.bundledEdgesIds = activation.bundledEdgeIds;
  message.dispatch();
}

void GraphViewModel::toggleExpand(qulonglong tokenId, bool expanded) {
  MessageGraphNodeExpand{static_cast<Id>(tokenId), !expanded}.dispatch();
}

void GraphViewModel::splitBundle(qulonglong tokenId) {
  MessageGraphNodeBundleSplit{static_cast<Id>(tokenId)}.dispatch();
}

void GraphViewModel::activateGroup(qulonglong tokenId, int groupType) {
  const GroupType type = groupingFromInt(groupType);
  if(type == GroupType::FILE_TYPE || type == GroupType::NAMESPACE) {
    MessageActivateNodes{static_cast<Id>(tokenId)}.dispatch();
  } else {
    MessageGraphNodeBundleSplit{static_cast<Id>(tokenId)}.dispatch();
  }
}

void GraphViewModel::hideNode(qulonglong tokenId) {
  MessageGraphNodeHide{static_cast<Id>(tokenId)}.dispatch();
}

void GraphViewModel::openInNewTab(qulonglong tokenId) {
  MessageTabOpenWith{static_cast<Id>(tokenId)}.dispatch();
}

void GraphViewModel::showOverview() {
  MessageActivateOverview{}.dispatch();
}

void GraphViewModel::applySnapshot(Snapshot snapshot) {
  mEdgeActivations = std::move(snapshot.edgeActivations);

  mBoundsX = snapshot.boundsX;
  mBoundsY = snapshot.boundsY;
  mBoundsWidth = snapshot.boundsWidth;
  mBoundsHeight = snapshot.boundsHeight;

  // Edges first: a node delegate that binds to the edge model must not see a half-updated pair.
  mEdges.reset(std::move(snapshot.edges));
  mNodes.reset(std::move(snapshot.nodes));

  Q_EMIT graphChanged();

  if(snapshot.centerActiveNode && snapshot.activeTokenId != 0) {
    for(const NodeItem& node : mNodes.items()) {
      if(node.tokenId == snapshot.activeTokenId) {
        Q_EMIT centerRequested(node.x, node.y, node.width, node.height);
        break;
      }
    }
  }
}

}    // namespace graph
