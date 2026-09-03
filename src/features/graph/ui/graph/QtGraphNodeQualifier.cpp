#include "graph/ui/graph/QtGraphNodeQualifier.h"

#include <QFont>
#include <QFontMetrics>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QVector2D>

#include "data/name/NameHierarchy.h"
#include "graph/messages/MessageActivateNodes.h"

QtGraphNodeQualifier::QtGraphNodeQualifier(const NameHierarchy& name) : m_qualifierName(name) {
  this->setAcceptHoverEvents(true);

  m_background = new QGraphicsRectItem(this);
  m_leftBorder = new QGraphicsRectItem(this);
  m_rightArrow = new QGraphicsPolygonItem(this);
  m_rightArrowSmall = new QGraphicsPolygonItem(this);

  QFont font;
  font.setFamily(GraphViewStyle::getFontNameForDataNode().c_str());
  font.setPixelSize(static_cast<int>(GraphViewStyle::getFontSizeOfQualifier()));
  font.setWeight(QFont::Normal);

  m_name = new QGraphicsSimpleTextItem(this);
  m_name->setFont(font);
  m_name->setText(QString::fromStdWString(name.getQualifiedName()));
}

QtGraphNodeQualifier::~QtGraphNodeQualifier() {}

bool QtGraphNodeQualifier::isQualifierNode() const {
  return true;
}

bool QtGraphNodeQualifier::setPosition(const QVector2D& pos) {
  const int width = QFontMetrics(m_name->font()).boundingRect(m_name->text()).width() + 10;
  const int height = QFontMetrics(m_name->font()).height() + 2;
  const int arrowWidth = static_cast<int>(height * 0.85);

  const qreal smallFactor = 0.5;
  const int arrowOffset = static_cast<int>(arrowWidth * smallFactor);

  // Qt's geometry API is qreal throughout, so do the arithmetic in qreal rather than promote a
  // float expression at every call.
  const QPointF position = pos.toPointF();
  const qreal left = position.x() - width - arrowWidth + arrowOffset;
  const qreal top = position.y() - height / 2.0;

  m_background->setRect(left, top, width, height);

  m_name->setPos(left + 6.0, top + 1.0);
  m_leftBorder->setRect(left, top, 2, height);

  QPolygonF poly;
  poly.append(QPointF(-arrowWidth, -height / 2 - 0.5));
  poly.append(QPointF(-arrowWidth, height / 2 + 0.5));
  poly.append(QPointF(0, 0));
  m_rightArrow->setPolygon(poly);
  m_rightArrow->setPos(position.x() + arrowOffset, position.y());

  QPolygonF polySmall;
  polySmall.append(QPointF(-arrowWidth * smallFactor, -height * smallFactor / 2.0));
  polySmall.append(QPointF(-arrowWidth * smallFactor, height * smallFactor / 2.0));
  polySmall.append(QPointF(0, 0));
  m_rightArrowSmall->setPolygon(polySmall);
  m_rightArrowSmall->setPos(position.x() + arrowOffset + 1.0, position.y());

  m_pos = pos;

  return true;
}

void QtGraphNodeQualifier::onClick() {
  hide();

  MessageActivateNodes msg;
  msg.addNode(0, m_qualifierName);
  msg.dispatch();
}

void QtGraphNodeQualifier::updateStyle() {
  GraphViewStyle::NodeStyle style = GraphViewStyle::getStyleOfQualifier();

  this->setBrush(Qt::transparent);
  this->setPen(QPen(Qt::transparent));

  m_background->setBrush(QColor(style.color.fill.c_str()));
  m_background->setPen(QPen(QColor(style.color.border.c_str()), 1));

  m_name->setBrush(QColor(style.color.text.c_str()));

  QRectF rect = m_leftBorder->rect();
  rect.setWidth(style.borderWidth);
  m_leftBorder->setRect(rect);

  m_leftBorder->setBrush(QColor(style.color.border.c_str()));
  m_leftBorder->setPen(QPen(Qt::transparent));

  m_rightArrow->setBrush(QColor(style.color.border.c_str()));
  m_rightArrow->setPen(QPen(Qt::transparent));

  m_rightArrowSmall->setBrush(QColor(style.color.border.c_str()));
  m_rightArrowSmall->setPen(QPen(Qt::transparent));

  hoverLeaveEvent(nullptr);
}

void QtGraphNodeQualifier::hoverEnterEvent(QGraphicsSceneHoverEvent* /*event*/) {
  const int width = QFontMetrics(m_name->font()).boundingRect(m_name->text()).width() + 10;
  const int height = QFontMetrics(m_name->font()).height() + 2;
  const int arrowWidth = static_cast<int>(height * 0.85);
  const qreal smallFactor = 0.5;
  const int arrowOffset = static_cast<int>(arrowWidth * smallFactor);
  const int offset = width + arrowWidth - arrowOffset;

  const QPointF position = m_pos.toPointF();
  setRect(position.x() - offset, position.y() - height / 2.0, width + arrowWidth, height);

  m_background->show();
  m_name->show();
  m_leftBorder->show();
  m_rightArrow->show();
  m_rightArrowSmall->hide();

  QPointF p = mapToScene(pos());

  // Make sure the qualifier is not cut off at the front edge of the screen
  QGraphicsView* graphicsView = scene()->views().at(0);
  QRectF sceneRect = graphicsView->mapToScene(graphicsView->rect()).boundingRect();
  if(p.x() - offset < sceneRect.x()) {
    p.setX(sceneRect.x() + offset);
  }

  this->setParentItem(nullptr);
  this->setPos(p);
  this->setZValue(100);
}

void QtGraphNodeQualifier::hoverLeaveEvent(QGraphicsSceneHoverEvent* /*event*/) {
  const int height = QFontMetrics(m_name->font()).height() + 2;
  const int arrowWidth = static_cast<int>(height * 0.85);
  const qreal smallFactor = 0.5;
  const int arrowOffset = static_cast<int>(arrowWidth * smallFactor);

  const QPointF position = m_pos.toPointF();
  setRect(position.x() - arrowWidth + arrowOffset, position.y() - height / 2.0, arrowWidth, height);

  m_background->hide();
  m_name->hide();
  m_leftBorder->hide();
  m_rightArrow->hide();
  m_rightArrowSmall->show();

  this->setParentItem(m_parentNode);
  this->setPos(QPointF(0, 0));
}
