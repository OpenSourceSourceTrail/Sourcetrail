#ifndef QT_COUNT_CIRCLE_ITEM_H
#define QT_COUNT_CIRCLE_ITEM_H

#include <QVector2D>

#include "QtRoundedRectItem.h"

class QtCountCircleItem : public QtRoundedRectItem {
public:
  QtCountCircleItem(QGraphicsItem* parent);
  virtual ~QtCountCircleItem();

  void setPosition(const QVector2D& pos);
  void setNumber(size_t number);
  void setStyle(QColor color, QColor fontColor, QColor borderColor, size_t borderWidth);

private:
  QGraphicsSimpleTextItem* m_number;
};

#endif    // QT_COUNT_CIRCLE_ITEM_H
