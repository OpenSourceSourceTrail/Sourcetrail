#include "QtGraphNodeQualifier.h"

#include <QFont>
#include <QFontMetrics>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QVector2D>

#include "NameHierarchy.h"
#include "type/graph/MessageActivateNodes.h"

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

  const float smallFactor = 0.5f;
  const int arrowOffset = static_cast<int>(static_cast<float>(arrowWidth) * smallFactor);

  m_background->setRect(pos.x() - static_cast<float>(width) - static_cast<float>(arrowWidth) + static_cast<float>(arrowOffset),
                        pos.y() - static_cast<float>(height) / 2.0f,
                        width,
                        height);

  m_name->setPos(pos.x() - static_cast<float>(width) - static_cast<float>(arrowWidth) + static_cast<float>(arrowOffset) + 6.0f,
                 pos.y() - static_cast<float>(height) / 2.0f + 1.0f);
  m_leftBorder->setRect(pos.x() - static_cast<float>(width) - static_cast<float>(arrowWidth) + static_cast<float>(arrowOffset),
                        pos.y() - static_cast<float>(height) / 2.0f,
                        2,
                        height);

  QPolygonF poly;
  poly.append(QPointF(-arrowWidth, -height / 2 - 0.5));
  poly.append(QPointF(-arrowWidth, height / 2 + 0.5));
  poly.append(QPointF(0, 0));
  m_rightArrow->setPolygon(poly);
  m_rightArrow->setPos(pos.x() + static_cast<float>(arrowOffset), pos.y());

  QPolygonF polySmall;
  polySmall.append(
      QPointF(-1.0f * static_cast<float>(arrowWidth) * smallFactor, -1.0f * static_cast<float>(height) * smallFactor / 2.0f));
  polySmall.append(QPointF(-1.0f * static_cast<float>(arrowWidth) * smallFactor, static_cast<float>(height) * smallFactor / 2.0f));
  polySmall.append(QPointF(0, 0));
  m_rightArrowSmall->setPolygon(polySmall);
  m_rightArrowSmall->setPos(pos.x() + static_cast<float>(arrowOffset) + 1.0f, pos.y());

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
  const float smallFactor = 0.5f;
  const int arrowOffset = static_cast<int>(static_cast<float>(arrowWidth) * smallFactor);
  const int offset = width + arrowWidth - arrowOffset;

  setRect(m_pos.x() - static_cast<float>(offset), m_pos.y() - static_cast<float>(height) / 2.0f, width + arrowWidth, height);

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
  const float smallFactor = 0.5f;
  const int arrowOffset = static_cast<int>(static_cast<float>(arrowWidth) * smallFactor);

  setRect(m_pos.x() - static_cast<float>(arrowWidth) + static_cast<float>(arrowOffset),
          m_pos.y() - static_cast<float>(height) / 2.0f,
          arrowWidth,
          height);

  m_background->hide();
  m_name->hide();
  m_leftBorder->hide();
  m_rightArrow->hide();
  m_rightArrowSmall->show();

  this->setParentItem(m_parentNode);
  this->setPos(QPointF(0, 0));
}
