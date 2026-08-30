#pragma once
#include <array>

#include <QPointF>
#include <QString>

#include "component/view/GraphViewStyle.h"

namespace graph {

/** A node's absolute bounds: left, top, right, bottom. */
struct Rect final {
  float left = 0;
  float top = 0;
  float right = 0;
  float bottom = 0;
};

enum class Route { Any, Horizontal, Vertical };

/**
 * Routes one edge and returns it as an SVG path.
 *
 * This is a port of QtLineItemBase::getPath and QtLineItemAngled::paint from the widget GUI. The
 * routing is not incidental -- which side of a box an edge leaves from, how it bends around the
 * enclosing class, where the corner radius shrinks because the segment is too short -- so it is
 * ported rather than reinvented, and the arcs go through QPainterPath so Qt's own arc-to-bezier
 * conversion produces the curve it always did.
 *
 * `ownerParent` / `targetParent` are the *outermost* ancestor of each end, which is what decides
 * where the line has to leave the enclosing box; pass the node's own rect when it has no parent.
 */
struct EdgePath final {
  QString path;
  QString arrowPath;    ///< Non-empty only for dashed styles, which draw a solid head over a dashed line.
};

[[nodiscard]] std::array<QPointF, 4> routeEdge(Rect owner,
                                               Rect target,
                                               Rect ownerParent,
                                               Rect targetParent,
                                               const GraphViewStyle::EdgeStyle& style,
                                               Route route,
                                               bool onFront,
                                               bool onBack,
                                               bool earlyBend);

[[nodiscard]] EdgePath buildEdgePath(const std::array<QPointF, 4>& poly, const GraphViewStyle::EdgeStyle& style, bool showArrow);

}    // namespace graph
