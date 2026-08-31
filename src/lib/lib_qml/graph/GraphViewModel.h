#pragma once
#include <map>
#include <memory>

#include <QObject>

#include <qqmlintegration.h>

#include "graph/GraphModels.h"
#include "graph/GraphSnapshot.h"

class Component;
class GraphController;
class QJSEngine;
class QQmlEngine;
class StorageAccess;

namespace graph {

class QmlGraphView;

/**
 * The graph panel's half of the application, on the QML side of the boundary.
 *
 * It owns a GraphController and the QmlGraphView that controller pushes into, which is the whole
 * of the reuse: nesting, bundling, grouping and placement stay in lib, where the engine daemon
 * drives the same code through its own headless view. QML gets two flat models and paints them.
 *
 * Every layout arrives on the message-bus thread and hops here through qml::postToGui, so the GUI
 * thread never lays out a graph and never blocks the bus while it repaints one.
 */
class GraphViewModel final : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  // Exposed as the base interface: QML only ever hands these to a Repeater, and registering the
  // concrete types would mean QML could name something it must never construct.
  Q_PROPERTY(QAbstractItemModel* nodes READ nodes CONSTANT)
  Q_PROPERTY(QAbstractItemModel* edges READ edges CONSTANT)

  Q_PROPERTY(qreal boundsX READ boundsX NOTIFY graphChanged)
  Q_PROPERTY(qreal boundsY READ boundsY NOTIFY graphChanged)
  Q_PROPERTY(qreal boundsWidth READ boundsWidth NOTIFY graphChanged)
  Q_PROPERTY(qreal boundsHeight READ boundsHeight NOTIFY graphChanged)
  Q_PROPERTY(bool empty READ empty NOTIFY graphChanged)

  /** 0 none, 1 by file, 2 by namespace -- a layout input the controller reads back while it works. */
  Q_PROPERTY(int grouping READ grouping WRITE setGrouping NOTIFY groupingChanged)

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
  explicit GraphViewModel(QObject* parent);
  ~GraphViewModel() override;

  static GraphViewModel* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

  /** Called by AppShell::setup(), once the message queue exists. */
  void attach(StorageAccess* storageAccess);
  void detach();

  [[nodiscard]] QAbstractItemModel* nodes();
  [[nodiscard]] QAbstractItemModel* edges();

  [[nodiscard]] qreal boundsX() const;
  [[nodiscard]] qreal boundsY() const;
  [[nodiscard]] qreal boundsWidth() const;
  [[nodiscard]] qreal boundsHeight() const;
  [[nodiscard]] bool empty() const;

  [[nodiscard]] int grouping() const;
  void setGrouping(int grouping);

  /** @name What QML does to the graph. All dispatch a message and return. @{ */
  Q_INVOKABLE void activateNode(qulonglong tokenId, bool isActive, bool multipleActive);
  Q_INVOKABLE void activateEdge(qulonglong edgeId);
  Q_INVOKABLE void toggleExpand(qulonglong tokenId, bool expanded);
  Q_INVOKABLE void splitBundle(qulonglong tokenId);
  Q_INVOKABLE void activateGroup(qulonglong tokenId, int groupType);
  Q_INVOKABLE void hideNode(qulonglong tokenId);
  Q_INVOKABLE void openInNewTab(qulonglong tokenId);
  Q_INVOKABLE void showOverview();
  /** @} */

  /**
   * Tells the controller how big the canvas is. BucketLayouter divides the viewport into columns,
   * so a wrong size here is a wrong layout, not just a wrong scroll position.
   */
  Q_INVOKABLE void setViewportSize(qreal width, qreal height);

Q_SIGNALS:
  void graphChanged();
  void groupingChanged();
  /** The layout asked to be centred on the active node; QML decides how to animate there. */
  void centerRequested(qreal x, qreal y, qreal width, qreal height);

private:
  void applySnapshot(Snapshot snapshot);

  static GraphViewModel* sInstance;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

  NodeModel mNodes;
  EdgeModel mEdges;

  std::shared_ptr<QmlGraphView> mView;
  std::shared_ptr<GraphController> mController;
  std::unique_ptr<Component> mComponent;

  qreal mBoundsX = 0;
  qreal mBoundsY = 0;
  qreal mBoundsWidth = 0;
  qreal mBoundsHeight = 0;

  std::map<Id, EdgeActivation> mEdgeActivations;

  qreal mViewportWidth = 0;
  qreal mViewportHeight = 0;
  int mGrouping = 0;
};

}    // namespace graph
