#include "QtCountCircleItem.h"

#include <QFont>
#include <QFontMetrics>
#include <QPen>

#include "GraphViewStyle.h"

QtCountCircleItem::QtCountCircleItem(QGraphicsItem* parent) : QtRoundedRectItem(parent) {
  this->setRadius(10);
  this->setAcceptHoverEvents(true);

  QFont font;
  font.setFamily(GraphViewStyle::getFontNameOfExpandToggleNode().c_str());
  font.setPixelSize(static_cast<int>(GraphViewStyle::getFontSizeOfCountCircle()));
  font.setWeight(QFont::Normal);

  m_number = new QGraphicsSimpleTextItem(this);
  m_number->setFont(font);
}

QtCountCircleItem::~QtCountCircleItem() {}

void QtCountCircleItem::setPosition(const QVector2D& pos) {
  qreal width = QFontMetrics(m_number->font()).boundingRect(m_number->text()).width();
  qreal height = QFontMetrics(m_number->font()).height();

  this->setRadius(height / 2 + 1);
  this->setRect(static_cast<qreal>(pos.x()) - std::max(width / 2.0 + 4.0, height / 2.0 + 1.0),
                static_cast<qreal>(pos.y()) - height / 2.0 - 1.0,
                std::max(width + 8.0, height + 2.0),
                height + 2);
  m_number->setPos(static_cast<qreal>(pos.x()) - width / 2.0, static_cast<qreal>(pos.y()) - height / 2.0);
}

void QtCountCircleItem::setNumber(size_t number) {
  const QString numberStr = QString::number(number);
  m_number->setText(numberStr);

  const QPointF center = this->rect().center();
  this->setPosition({static_cast<float>(center.x()), static_cast<float>(center.y())});
}

void QtCountCircleItem::setStyle(QColor color, QColor fontColor, QColor borderColor, size_t borderWidth) {
  this->setBrush(color);
  this->setPen(QPen(borderColor, static_cast<qreal>(borderWidth)));

  m_number->setBrush(fontColor);
}
