#ifndef QT_GRAPH_NODE_COMPONENT_CLICKABLE
#define QT_GRAPH_NODE_COMPONENT_CLICKABLE

#include <QVector2D>

#include "QtGraphNodeComponent.h"

class QtGraphNodeComponentClickable : public QtGraphNodeComponent {
public:
  QtGraphNodeComponentClickable(QtGraphNode* graphNode);
  virtual ~QtGraphNodeComponentClickable();

  virtual void nodeMousePressEvent(QGraphicsSceneMouseEvent* event);
  virtual void nodeMouseMoveEvent(QGraphicsSceneMouseEvent* event);
  virtual void nodeMouseReleaseEvent(QGraphicsSceneMouseEvent* event);

private:
  QVector2D m_mousePos;
  bool m_mouseMoved;
};

#endif    // QT_GRAPH_NODE_COMPONENT_CLICKABLE
