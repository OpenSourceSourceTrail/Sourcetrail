#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "component/view/GraphView.h"
#include "graph/GraphSnapshot.h"

namespace graph {

/**
 * The GraphView the controller pushes layouts into, in the process that owns the index.
 *
 * It paints nothing. GraphController hands it a DummyNode tree with relative positions and a
 * DummyEdge list with no geometry at all; this turns both into a flat, absolutely positioned,
 * fully styled Snapshot and hands that to a callback. The callback runs on whatever thread the
 * message bus used, so the snapshot is a value the GUI thread can take ownership of.
 *
 * Style resolution and edge routing happen here rather than in QML for the same reason the layout
 * does: GraphViewStyle and the routing rules are the definition of what a Sourcetrail graph looks
 * like, and a second implementation in QML would drift from the one the engine still serves over
 * /api/v1/graph/layout.
 */
class QmlGraphView final : public GraphView {
public:
  using SnapshotHandler = std::function<void(Snapshot)>;

  explicit QmlGraphView(SnapshotHandler onSnapshot);
  ~QmlGraphView() override;

  /** Layout inputs the controller reads back while it works. Set from the GUI before activating. */
  void setLayoutInputs(Vec2f viewSize, GroupType grouping);

  void refreshView() override;

  void rebuildGraph(std::shared_ptr<Graph> graph,
                    const std::vector<std::shared_ptr<DummyNode>>& nodes,
                    const std::vector<std::shared_ptr<DummyEdge>>& edges,
                    const GraphParams params) override;

  void clear() override;

  void coFocusTokenIds(const std::vector<Id>& focusedTokenIds) override;
  void deCoFocusTokenIds(const std::vector<Id>& defocusedTokenIds) override;

  void resizeView() override;

  [[nodiscard]] Vec2f getViewSize() const override;
  [[nodiscard]] GroupType getGrouping() const override;

  void scrollToValues(int xValue, int yValue) override;
  void activateEdge(Id edgeId) override;

  void setNavigationFocus(bool focus) override;
  [[nodiscard]] bool hasNavigationFocus() const override;

  // ScreenSearchResponder -- screen search lands with its own view-model.
  [[nodiscard]] bool isVisible() const override;
  void findMatches(ScreenSearchSender* sender, const std::wstring& query) override;
  void activateMatch(size_t matchIndex) override;
  void deactivateMatch(size_t matchIndex) override;
  void clearMatches() override;

private:
  SnapshotHandler mOnSnapshot;

  Vec2f mViewSize;
  GroupType mGrouping = GroupType::NONE;

  bool mNavigationFocus = false;
};

}    // namespace graph
