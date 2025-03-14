#include "QtGraphNodeComponentClickable.h"

#include <QGraphicsSceneEvent>
#include <QVector2D>

#include "QtGraphNode.h"

QtGraphNodeComponentClickable::QtGraphNodeComponentClickable(QtGraphNode* graphNode)
    : QtGraphNodeComponent(graphNode), m_mousePos{}, m_mouseMoved(false) {}

QtGraphNodeComponentClickable::~QtGraphNodeComponentClickable() {}

void QtGraphNodeComponentClickable::nodeMousePressEvent(QGraphicsSceneMouseEvent* event) {
  if(event->button() != Qt::LeftButton && event->button() != Qt::MiddleButton) {
    return;
  }

  m_mousePos = {static_cast<float>(event->scenePos().x()), static_cast<float>(event->scenePos().y())};
  m_mouseMoved = false;

  if(event->button() == Qt::MiddleButton) {
    event->accept();
  }
}

void QtGraphNodeComponentClickable::nodeMouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  QVector2D mousePos{static_cast<float>(event->scenePos().x()), static_cast<float>(event->scenePos().y())};

  if((mousePos - m_mousePos).length() > 3.0f) {
    m_mouseMoved = true;
  }
}

void QtGraphNodeComponentClickable::nodeMouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  if(event->button() != Qt::LeftButton && event->button() != Qt::MiddleButton) {
    return;
  }

  if(!m_mouseMoved) {
    if(event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier && event->button() == Qt::LeftButton) {
      m_graphNode->onMiddleClick();
    } else if(event->modifiers() & Qt::ShiftModifier && event->button() == Qt::LeftButton) {
      m_graphNode->onCollapseExpand();
    } else if(event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::AltModifier &&
              event->button() == Qt::LeftButton) {
      m_graphNode->onShowDefinition(false);
    } else if(event->modifiers() & Qt::ControlModifier && event->button() == Qt::LeftButton) {
      m_graphNode->onShowDefinition(true);
    } else if(event->modifiers() & Qt::AltModifier && event->button() == Qt::LeftButton) {
      m_graphNode->onHide();
    } else {
      if(event->button() == Qt::MiddleButton) {
        m_graphNode->onMiddleClick();
      } else {
        m_graphNode->onClick();
      }
    }
    event->accept();
  }
}
