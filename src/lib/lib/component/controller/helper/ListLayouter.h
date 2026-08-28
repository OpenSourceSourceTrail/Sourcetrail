#ifndef LIST_LAYOUTER_H
#define LIST_LAYOUTER_H

#include <memory>
#include <vector>

#include "LayoutRect.h"
#include "Vec2f.h"

struct DummyNode;

class ListLayouter {
public:
  static void layoutRow(std::vector<std::shared_ptr<DummyNode>>* nodes, int gap);
  static void layoutColumn(std::vector<std::shared_ptr<DummyNode>>* nodes, int gap);

  static void layoutMultiColumn(Vec2f viewSize, std::vector<std::shared_ptr<DummyNode>>* nodes);
  static void layoutSquare(std::vector<std::shared_ptr<DummyNode>>* nodes, int maxWidth);
  static void layoutSkewed(std::vector<std::shared_ptr<DummyNode>>* nodes, int gapX, int gapY, int maxWidth);

  static LayoutRect boundingRect(const std::vector<std::shared_ptr<DummyNode>>& nodes);
  static Vec2f offsetNodes(const std::vector<std::shared_ptr<DummyNode>>& nodes, int top, int left);

private:
  static void layoutSimple(std::vector<std::shared_ptr<DummyNode>>* nodes, int gapX, int gapY, bool horizontal);
  static bool layoutSquareInternal(std::vector<std::shared_ptr<DummyNode>>& visibleNodes, const Vec2f& maxSize, const Vec2f& gap);
};

#endif    // LIST_LAYOUTER_H
