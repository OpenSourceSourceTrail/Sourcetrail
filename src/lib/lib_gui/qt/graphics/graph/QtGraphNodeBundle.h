#pragma once

#include "QtGraphNode.h"

class QtCountCircleItem;

class QtGraphNodeBundle : public QtGraphNode {
  Q_OBJECT
public:
  QtGraphNodeBundle(
      GraphFocusHandler* focusHandler, Id tokenId, size_t nodeCount, NodeType type, const std::wstring& name, bool interactive);
  ~QtGraphNodeBundle() override;

  // QtGraphNode implementation
  [[nodiscard]] bool isBundleNode() const override;
  [[nodiscard]] Id getTokenId() const override;

  void onClick() override;
  void updateStyle() override;

protected:
  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
  QtCountCircleItem* m_circle;
  Id m_tokenId;
  NodeType m_type;
};