#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "GlobalId.hpp"

namespace sourcetrail {
class GraphLayoutRequest;
class GraphLayoutResponse;
}    // namespace sourcetrail

class Component;
class GraphController;
class HeadlessGraphView;
class StorageAccess;

/**
 * Runs the graph layout the Qt view drives, without a Qt view.
 *
 * The layout pipeline -- GraphController builds a DummyNode tree, BucketLayouter/ListLayouter place
 * it -- already lives in `lib` and is toolkit-agnostic; the only things it wants from a view are a
 * viewport size, a grouping mode and somewhere to hand the finished tree. HeadlessGraphView
 * supplies those, so a client that cannot link the layouters gets positions over the wire instead
 * of reimplementing them.
 *
 * The controller is long-lived, exactly as it is in the Qt app: expand/collapse state lives in its
 * DummyNode tree and nowhere else, so a fresh controller per request would forget which nodes the
 * user had opened. It is built on the first layout request rather than in the constructor, because
 * GraphController registers itself with IMessageQueue -- which a caller that never asks for a
 * layout (the client test suite, for one) has no reason to have stood up.
 * ponytail: one controller, so one client's graph state. Key it by session if the engine ever
 * serves two front ends at once.
 */
class GraphLayoutService {
public:
  explicit GraphLayoutService(StorageAccess* storageAccess);
  ~GraphLayoutService();

  GraphLayoutService(const GraphLayoutService&) = delete;
  GraphLayoutService& operator=(const GraphLayoutService&) = delete;

  /** Lays out the graph for `request.token_ids` and returns the placed tree. */
  sourcetrail::GraphLayoutResponse layout(const sourcetrail::GraphLayoutRequest& request);

private:
  /** Builds the controller and its headless view on first use. False when there is no message queue. */
  bool ensureController();

  StorageAccess* mStorageAccess;
  std::shared_ptr<HeadlessGraphView> mView;
  std::shared_ptr<GraphController> mController;
  std::unique_ptr<Component> mComponent;

  // GraphViewStyle's metrics and the controller's tree are both process-global state, and the HTTP
  // server serves each connection on its own thread.
  // ponytail: one lock for the whole layout; graph requests are user-paced, not a throughput path.
  std::mutex mMutex;
};
