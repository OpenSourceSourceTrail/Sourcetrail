#include "graph/EdgeRouter.h"

#include <cmath>
#include <map>

#include <QPainterPath>
#include <QPointF>
#include <QVector2D>
#include <QVector4D>

namespace graph {

namespace {

QVector4D toVector(const Rect& rect) {
  return {rect.left, rect.top, rect.right, rect.bottom};
}

/**
 * The four points an edge can leave a box from: top, right, bottom, left.
 *
 * `in` supplies the centre along one axis, `out` the edge along the other -- passing the node's own
 * rect for both gives the points on the node, passing its outermost ancestor gives the points where
 * the line clears the enclosing box.
 */
void pivotPoints(std::array<QVector2D, 4>& points, const QVector4D& in, const QVector4D& out, int offset) {
  constexpr float Half = 0.5F;
  const auto centreX = in.x() + (in.z() - in.x()) * Half + static_cast<float>(offset);
  const auto centreY = in.y() + (in.w() - in.y()) * Half + static_cast<float>(offset);

  points[0] = {centreX, out.y()};
  points[1] = {out.z(), centreY};
  points[2] = {centreX, out.w()};
  points[3] = {out.x(), centreY};
}

/** Which way b lies from a: 0 up, 1 right, 2 down, 3 left. */
int directionOf(QPointF from, QPointF to) {
  if(from.x() != to.x()) {
    return from.x() < to.x() ? 1 : 3;
  }
  return from.y() < to.y() ? 2 : 0;
}

/** Nudges an offsetted pivot outwards along the side it belongs to. */
void applySideOffset(QPoint& point, int side, float offset) {
  switch(side) {
  case 0:
    point.setY(static_cast<int>(static_cast<float>(point.y()) - offset));
    break;
  case 1:
    point.setX(static_cast<int>(static_cast<float>(point.x()) + offset));
    break;
  case 2:
    point.setY(static_cast<int>(static_cast<float>(point.y()) + offset));
    break;
  case 3:
    point.setX(static_cast<int>(static_cast<float>(point.x()) - offset));
    break;
  default:
    break;
  }
}

QPoint toPoint(const QVector2D& vector) {
  return {static_cast<int>(vector.x()), static_cast<int>(vector.y())};
}

void appendArrow(QPainterPath& path,
                 QPainterPath* detachedArrow,
                 const std::array<QPointF, 4>& poly,
                 const GraphViewStyle::EdgeStyle& style) {
  const int direction = directionOf(poly.at(1), poly.at(0));

  const QPointF tip = poly.at(0);
  QPointF toBack;
  QPointF toLeft;

  switch(direction) {
  case 0:
    toBack.setY(style.arrowLength);
    toLeft.setX(-style.arrowWidth / 2.0);
    break;
  case 1:
    toBack.setX(-style.arrowLength);
    toLeft.setY(-style.arrowWidth / 2.0);
    break;
  case 2:
    toBack.setY(-style.arrowLength);
    toLeft.setX(style.arrowWidth / 2.0);
    break;
  default:
    toBack.setX(style.arrowLength);
    toLeft.setY(style.arrowWidth / 2.0);
    break;
  }

  if(style.arrowClosed) {
    path.lineTo(tip + toBack);
    path.moveTo(tip);
  } else {
    path.lineTo(tip);
  }

  QPainterPath* head = &path;
  if(detachedArrow != nullptr) {
    head = detachedArrow;
    head->moveTo(tip);
  }

  head->lineTo(tip + toBack + toLeft);
  if(style.arrowClosed) {
    head->lineTo(tip + toBack - toLeft);
  } else {
    head->moveTo(tip + toBack - toLeft);
  }
  head->lineTo(tip);
}

/**
 * QPainterPath stores arcs already flattened to cubic beziers, so this only has to spell three
 * element kinds. Coordinates are absolute, matching the node positions.
 */
QString toSvg(const QPainterPath& path) {
  QString out;
  out.reserve(path.elementCount() * 24);

  for(int index = 0; index < path.elementCount(); ++index) {
    const QPainterPath::Element element = path.elementAt(index);
    switch(element.type) {
    case QPainterPath::MoveToElement:
      out += QStringLiteral("M%1 %2").arg(element.x).arg(element.y);
      break;
    case QPainterPath::LineToElement:
      out += QStringLiteral("L%1 %2").arg(element.x).arg(element.y);
      break;
    case QPainterPath::CurveToElement: {
      // A CurveTo is always followed by exactly two CurveToData elements.
      const QPainterPath::Element control2 = path.elementAt(index + 1);
      const QPainterPath::Element end = path.elementAt(index + 2);
      out +=
          QStringLiteral("C%1 %2 %3 %4 %5 %6").arg(element.x).arg(element.y).arg(control2.x).arg(control2.y).arg(end.x).arg(end.y);
      index += 2;
      break;
    }
    case QPainterPath::CurveToDataElement:
      break;
    }
  }
  return out;
}

}    // namespace

std::array<QPointF, 4> routeEdge(Rect owner,
                                 Rect target,
                                 Rect ownerParent,
                                 Rect targetParent,
                                 const GraphViewStyle::EdgeStyle& style,
                                 Route route,
                                 bool onFront,
                                 bool onBack,
                                 bool earlyBend) {
  // The one-pixel outset the widget line item applied before routing; without it edges that leave
  // sideways land exactly on the border and disappear under it.
  owner.left -= 1;
  owner.right += 1;
  target.left -= 1;
  target.right += 1;

  const QVector4D ownerRect = toVector(owner);
  const QVector4D targetRect = toVector(target);
  const QVector4D ownerParentRect = toVector(ownerParent);
  const QVector4D targetParentRect = toVector(targetParent);

  const Vec2f& originOffset = style.originOffset;
  const Vec2f& targetOffset = style.targetOffset;

  std::array<QVector2D, 4> ownerPivots{};
  pivotPoints(ownerPivots, ownerRect, ownerParentRect, static_cast<int>(originOffset.y));

  std::array<QVector2D, 4> targetPivots{};
  pivotPoints(targetPivots, targetRect, targetParentRect, static_cast<int>(targetOffset.y));

  int originSide = -1;
  int targetSide = -1;

  // Pick the closest pair of opposite sides, honouring a forced route.
  std::map<int, float> distances;
  if(onFront) {
    originSide = 3;
    targetSide = 3;
  } else if(onBack) {
    originSide = 1;
    targetSide = 1;
  } else {
    float best = -1;
    for(int i = 0; i < 4; ++i) {
      for(int j = 0; j < 4; ++j) {
        if(route == Route::Horizontal && (i % 2 == 0 || j % 2 == 0)) {
          continue;
        }
        if(route == Route::Vertical && (i % 2 == 1 || j % 2 == 1)) {
          continue;
        }
        if(i % 2 != j % 2) {
          continue;
        }

        const float distance = (ownerPivots.at(static_cast<size_t>(i)) - targetPivots.at(static_cast<size_t>(j))).length();
        distances.emplace((i << 2) + j, distance);

        if(best < 0 || distance < best) {
          best = distance;
          originSide = i;
          targetSide = j;
        }
      }
    }
  }

  std::array<QVector2D, 4> ownerPoints{};
  pivotPoints(ownerPoints, ownerRect, ownerRect, static_cast<int>(originOffset.y));

  std::array<QVector2D, 4> targetPoints{};
  pivotPoints(targetPoints, targetRect, targetRect, static_cast<int>(targetOffset.y));

  QPoint pointA = toPoint(targetPoints.at(static_cast<size_t>(targetSide)));
  QPoint pointD = toPoint(ownerPoints.at(static_cast<size_t>(originSide)));
  QPoint pointB = toPoint(targetPivots.at(static_cast<size_t>(targetSide)));
  QPoint pointC = toPoint(ownerPivots.at(static_cast<size_t>(originSide)));

  applySideOffset(pointB, targetSide, targetOffset.x);
  applySideOffset(pointC, originSide, originOffset.x);

  // One of the two bend points has to move so the line does not double back on itself.
  if(targetSide != originSide) {
    if((targetSide == 1 && pointB.x() < pointC.x()) || (originSide == 1 && pointB.x() > pointC.x())) {
      if(earlyBend) {
        pointB.setX(pointC.x());
      } else {
        pointC.setX(pointB.x());
      }
    } else if((targetSide == 2 && pointB.y() < pointC.y()) || (originSide == 2 && pointB.y() > pointC.y())) {
      if(earlyBend) {
        pointB.setY(pointC.y());
      } else {
        pointC.setY(pointB.y());
      }
    } else if((targetSide == 3 && pointB.x() < pointC.x()) || (originSide == 3 && pointB.x() > pointC.x()) ||
              (targetSide == 0 && pointB.y() < pointC.y()) || (originSide == 0 && pointB.y() > pointC.y())) {
      // Both ends face away from each other; flip whichever end has the shorter alternative.
      const float viaTarget = distances[(originSide << 2) + ((targetSide + 2) % 4)];
      const float viaOrigin = distances[(((originSide + 2) % 4) << 2) + targetSide];

      if(viaTarget < viaOrigin) {
        targetSide = (targetSide + 2) % 4;
        pointA = toPoint(targetPoints.at(static_cast<size_t>(targetSide)));
        pointB = toPoint(targetPivots.at(static_cast<size_t>(targetSide)));
        applySideOffset(pointB, targetSide, targetOffset.x);
      } else {
        originSide = (originSide + 2) % 4;
        pointD = toPoint(ownerPoints.at(static_cast<size_t>(originSide)));
        pointC = toPoint(ownerPivots.at(static_cast<size_t>(originSide)));
        applySideOffset(pointC, originSide, originOffset.x);
      }
    }
  }

  // Square off the middle segment.
  if(targetSide % 2 == 1) {
    if(targetSide == originSide && ((targetSide == 1 && pointB.x() < pointC.x()) || (targetSide == 3 && pointB.x() > pointC.x()))) {
      pointB.setX(pointC.x());
    } else {
      pointC.setX(pointB.x());
    }
  } else {
    if(targetSide == originSide && ((targetSide == 0 && pointB.y() > pointC.y()) || (targetSide == 2 && pointB.y() < pointC.y()))) {
      pointB.setY(pointC.y());
    } else {
      pointC.setY(pointB.y());
    }
  }

  // Fan parallel edges apart so a bundle does not collapse into one line.
  if(targetSide % 2 == 0) {
    int offset = style.verticalOffset;
    if(pointC.x() > pointB.x()) {
      offset *= -1;
    }
    pointB.setY(pointB.y() + offset);
    pointC.setY(pointC.y() + offset);
  } else {
    int offset = style.verticalOffset;
    if(pointC.y() > pointB.y()) {
      offset *= -1;
    }
    pointB.setX(pointB.x() + offset);
    pointC.setX(pointC.x() + offset);
  }

  return {QPointF{pointA}, QPointF{pointB}, QPointF{pointC}, QPointF{pointD}};
}

EdgePath buildEdgePath(const std::array<QPointF, 4>& poly, const GraphViewStyle::EdgeStyle& style, bool showArrow) {
  QPainterPath path;
  QPainterPath detachedArrow;

  // Walked from the owner end (index 3) towards the target (index 0), which is where the arrow is.
  auto index = static_cast<int>(poly.size()) - 1;
  path.moveTo(poly.at(static_cast<size_t>(index)));

  int radius = style.cornerRadius;
  int direction = directionOf(poly.at(static_cast<size_t>(index)), poly.at(static_cast<size_t>(index - 1)));

  while(index > 1) {
    --index;

    QPointF cornerIn = poly.at(static_cast<size_t>(index));
    QPointF cornerOut = poly.at(static_cast<size_t>(index));
    const QPointF next = poly.at(static_cast<size_t>(index - 1));

    const int newDirection = directionOf(cornerIn, next);
    const int radiusIn = radius;
    int radiusOut = style.cornerRadius;

    // A corner cannot be rounder than half the segment it turns into.
    if(index != 1) {
      if(direction % 2 == 1 && std::abs(cornerIn.y() - next.y()) < 2 * radiusOut) {
        radiusOut = static_cast<int>(std::abs(cornerIn.y() - next.y()) / 2);
      } else if(direction % 2 == 0 && std::abs(cornerIn.x() - next.x()) < 2 * radiusOut) {
        radiusOut = static_cast<int>(std::abs(cornerIn.x() - next.x()) / 2);
      }
    }

    switch(direction) {
    case 0:
      cornerIn.setY(cornerIn.y() + radiusIn);
      break;
    case 1:
      cornerIn.setX(cornerIn.x() - radiusIn);
      break;
    case 2:
      cornerIn.setY(cornerIn.y() - radiusIn);
      break;
    default:
      cornerIn.setX(cornerIn.x() + radiusIn);
      break;
    }

    switch(newDirection) {
    case 0:
      cornerOut.setY(cornerOut.y() - radiusOut);
      break;
    case 1:
      cornerOut.setX(cornerOut.x() + radiusOut);
      break;
    case 2:
      cornerOut.setY(cornerOut.y() + radiusOut);
      break;
    default:
      cornerOut.setX(cornerOut.x() - radiusOut);
      break;
    }

    path.lineTo(cornerIn);

    switch(direction) {
    case 0:
      if(newDirection == 1) {
        path.arcTo(cornerIn.x(), cornerOut.y(), 2.0 * radiusOut, 2.0 * radiusIn, 180, -90);
      } else if(newDirection == 3) {
        path.arcTo(cornerOut.x() - radiusOut, cornerIn.y() - radiusIn, 2.0 * radiusOut, 2.0 * radiusIn, 0, 90);
      }
      break;
    case 1:
      if(newDirection == 0) {
        path.arcTo(cornerIn.x() - radiusIn, cornerOut.y() - radiusOut, 2.0 * radiusIn, 2.0 * radiusOut, -90, 90);
      } else if(newDirection == 2) {
        path.arcTo(cornerIn.x() - radiusIn, cornerIn.y(), 2.0 * radiusIn, 2.0 * radiusOut, 90, -90);
      }
      break;
    case 2:
      if(newDirection == 1) {
        path.arcTo(cornerIn.x(), cornerIn.y() - radiusIn, 2.0 * radiusOut, 2.0 * radiusIn, 180, 90);
      } else if(newDirection == 3) {
        path.arcTo(cornerOut.x() - radiusOut, cornerIn.y() - radiusIn, 2.0 * radiusOut, 2.0 * radiusIn, 0, -90);
      }
      break;
    default:
      if(newDirection == 0) {
        path.arcTo(cornerOut.x(), cornerOut.y() - radiusOut, 2.0 * radiusIn, 2.0 * radiusOut, -90, -90);
      } else if(newDirection == 2) {
        path.arcTo(cornerOut.x(), cornerIn.y(), 2.0 * radiusIn, 2.0 * radiusOut, 90, 90);
      }
      break;
    }

    direction = newDirection;
    radius = radiusOut;
  }

  EdgePath result;
  if(showArrow) {
    appendArrow(path, style.dashed ? &detachedArrow : nullptr, poly, style);
    if(style.dashed) {
      result.arrowPath = toSvg(detachedArrow);
    }
  } else {
    path.lineTo(poly.at(0));
  }

  result.path = toSvg(path);
  return result;
}

}    // namespace graph
