#pragma once
#include "AccessKind.h"
#include "QtGraphNode.h"

class QtGraphNodeAccess : public QtGraphNode {
  Q_OBJECT
public:
  explicit QtGraphNodeAccess(AccessKind accessKind);
  ~QtGraphNodeAccess() override;

  [[nodiscard]] AccessKind getAccessKind() const;

  // QtGraphNode implementation
  [[nodiscard]] bool isAccessNode() const override;

  void addSubNode(QtGraphNode* node) override;
  void updateStyle() override;

  void hideLabel();

protected:
  void matchName(const std::wstring& /*query*/, std::vector<QtGraphNode*>* /*matchedNodes*/) override {}

private:
  AccessKind m_accessKind;

  QGraphicsPixmapItem* m_accessIcon;
  int m_accessIconSize;
};