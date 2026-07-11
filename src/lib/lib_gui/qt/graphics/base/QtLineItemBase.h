#ifndef QT_LINE_ITEM_BASE_H
#define QT_LINE_ITEM_BASE_H

#include <QGraphicsItem>
#include <QVector2D>
#include <QVector4D>

#include "GraphViewStyle.h"

class QtLineItemBase : public QGraphicsLineItem {
public:
  enum Route { ROUTE_ANY, ROUTE_HORIZONTAL, ROUTE_VERTICAL };

  QtLineItemBase(QGraphicsItem* parent);
  virtual ~QtLineItemBase();

  void updateLine(const QVector4D& ownerRect,
                  const QVector4D& targetRect,
                  const QVector4D& ownerParentRect,
                  const QVector4D& targetParentRect,
                  const GraphViewStyle::EdgeStyle& style,
                  size_t weight,
                  bool showArrow);

  void setRoute(Route route);

  void setOnFront(bool front);
  void setOnBack(bool back);
  void setEarlyBend(bool earlyBend);

protected:
  QPolygon getPath() const;
  int getDirection(QPointF a, QPointF b) const;

  QRectF getArrowBoundingRect(const QPolygon& poly) const;
  void drawArrow(const QPolygon& poly, QPainterPath* path, QPainterPath* arrowPath = nullptr) const;

  void getPivotPoints(QVector2D* p, const QVector4D& in, const QVector4D& out, int offset, bool target) const;

  GraphViewStyle::EdgeStyle m_style;
  bool m_showArrow;

  bool m_onFront;
  bool m_onBack;
  bool m_earlyBend;

  Route m_route;

private:
  QVector4D m_ownerRect;
  QVector4D m_targetRect;

  QVector4D m_ownerParentRect;
  QVector4D m_targetParentRect;

  mutable QPolygon m_polygon;
};

#endif    // QT_LINE_ITEM_BASE_H
