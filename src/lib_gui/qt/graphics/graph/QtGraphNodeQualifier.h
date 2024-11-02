#pragma once

#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QVector2D>

#include "NameHierarchy.h"
#include "QtGraphNode.h"

class QtGraphNodeQualifier : public QtGraphNode {
  Q_OBJECT
public:
  QtGraphNodeQualifier(const NameHierarchy& name);
  virtual ~QtGraphNodeQualifier();

  // QtGraphNode implementation
  virtual bool isQualifierNode() const;

  virtual bool setPosition(const QVector2D& pos);

  virtual void onClick();
  virtual void updateStyle();

protected:
  virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);

  virtual void matchName(const std::wstring& /*query*/, std::vector<QtGraphNode*>* /*matchedNodes*/) {}

private:
  const NameHierarchy m_qualifierName;

  QGraphicsRectItem* m_background;
  QGraphicsRectItem* m_leftBorder;
  QGraphicsPolygonItem* m_rightArrow;
  QGraphicsPolygonItem* m_rightArrowSmall;
  QGraphicsSimpleTextItem* m_name;

  QVector2D m_pos;
};