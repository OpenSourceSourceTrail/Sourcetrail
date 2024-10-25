#ifndef QT_GRAPH_NODE_COMPONENT_MOVEABLE
#define QT_GRAPH_NODE_COMPONENT_MOVEABLE

#include <QVector2D>

#include "QtGraphNodeComponent.h"

class QtGraphNodeComponentMoveable : public QtGraphNodeComponent {
public:
  QtGraphNodeComponentMoveable(QtGraphNode* graphNode);
  virtual ~QtGraphNodeComponentMoveable();

  virtual void nodeMousePressEvent(QGraphicsSceneMouseEvent* event);
  virtual void nodeMouseMoveEvent(QGraphicsSceneMouseEvent* event);
  virtual void nodeMouseReleaseEvent(QGraphicsSceneMouseEvent* event);

private:
  QVector2D m_mouseOffset;
  QVector2D m_oldPos;
};

#endif    // QT_GRAPH_NODE_COMPONENT_MOVEABLE
