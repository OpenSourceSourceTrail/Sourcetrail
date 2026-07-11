#include "ListLayouter.h"
// STL
#include <algorithm>
#include <cmath>
#include <limits>

// internal
#include "DummyNode.h"
#include "GraphViewStyle.h"

void ListLayouter::layoutRow(std::vector<std::shared_ptr<DummyNode>>* nodes, int gap) {
  layoutSimple(nodes, gap, 0, true);
}

void ListLayouter::layoutColumn(std::vector<std::shared_ptr<DummyNode>>* nodes, int gap) {
  layoutSimple(nodes, 0, gap, false);
}

void ListLayouter::layoutMultiColumn(Vec2f viewSize, std::vector<std::shared_ptr<DummyNode>>* nodes) {
  size_t colsFinal = {};
  std::vector<int> maxWidthsFinal;

  int gapX = GraphViewStyle::s_gridCellSize + 2 * GraphViewStyle::s_gridCellPadding;
  int gapY = GraphViewStyle::s_gridCellPadding;

  std::vector<std::shared_ptr<DummyNode>> visibleNodes;
  for(auto node : *nodes) {
    if(node->getsLayouted()) {
      visibleNodes.push_back(node);
    }
  }

  for(size_t cols = 1; cols <= 10; cols++) {
    std::vector<int> maxWidths = std::vector<int>(cols, 0);
    size_t nodesPerCol = (cols == 1 ? visibleNodes.size() :
                                      static_cast<size_t>(std::ceil(double(visibleNodes.size() + cols - 1) / double(cols))));

    int maxHeight = 0;
    int height = -gapY;

    for(size_t i = 0; i < visibleNodes.size(); i++) {
      size_t j = i / nodesPerCol;

      if(i % nodesPerCol == 0) {
        height = -gapY;
      }
      height += static_cast<int>(visibleNodes[i]->size.y) + gapY;

      maxWidths[j] = std::max(static_cast<int>(visibleNodes[i]->size.x), maxWidths[j]);
      maxHeight = std::max(height, maxHeight);
    }

    int width = -gapX;
    for(size_t j = 0; j < cols; j++) {
      width += maxWidths[j] + gapX;
    }

    if(static_cast<float>(width) > viewSize.x) {
      if(!maxWidthsFinal.size()) {
        colsFinal = 1;
        maxWidthsFinal = maxWidths;
      }
      break;
    }

    colsFinal = cols;
    maxWidthsFinal = maxWidths;

    if(static_cast<float>(height) < viewSize.y) {
      break;
    }
  }

  int x = 0;
  int y = 0;

  size_t nodesPerCol = (colsFinal == 1 ?
                            visibleNodes.size() :
                            static_cast<size_t>(std::ceil(double(visibleNodes.size() + colsFinal - 1) / double(colsFinal))));
  std::shared_ptr<DummyNode> lastTextNode;

  for(size_t i = 0; i < visibleNodes.size(); i++) {
    size_t j = i / nodesPerCol;
    if(j != 0 && j != colsFinal && i % nodesPerCol == 0) {
      if(lastTextNode && !visibleNodes[i]->isTextNode()) {
        std::shared_ptr<DummyNode> textNode = std::make_shared<DummyNode>(*lastTextNode.get());

        if(visibleNodes[i - 1] == lastTextNode) {
          lastTextNode->visible = false;
        } else if(textNode->name.size() == 1) {
          textNode->name += L"..";
        }

        visibleNodes.insert(visibleNodes.begin() + static_cast<long>(i), textNode);
        nodes->push_back(textNode);
        lastTextNode.reset();
        i--;
        continue;
      }

      y = 0;
      x += maxWidthsFinal[j - 1] + gapX;
    }

    visibleNodes[i]->position.x = static_cast<float>(x);
    visibleNodes[i]->position.y = static_cast<float>(y);

    y += static_cast<int>(visibleNodes[i]->size.y + static_cast<float>(gapY));

    if(visibleNodes[i]->isTextNode()) {
      lastTextNode = visibleNodes[i];
    }
  }
}

void ListLayouter::layoutSquare(std::vector<std::shared_ptr<DummyNode>>* nodes, int maxWidth) {
  int gapX = GraphViewStyle::s_gridCellSize + 2 * GraphViewStyle::s_gridCellPadding;
  int gapY = GraphViewStyle::s_gridCellPadding;

  std::vector<std::shared_ptr<DummyNode>> visibleNodes;
  for(auto node : *nodes) {
    if(node->getsLayouted()) {
      visibleNodes.push_back(node);
    }
  }

  int totalHeight = 0;
  for(size_t i = 0; i < visibleNodes.size(); i++) {
    totalHeight += static_cast<int>(visibleNodes[i]->size.y + static_cast<float>(gapY));
  }

  int diff = -1;
  int cols = 1;

  for(int i = cols; i < 100; i++) {
    if(layoutSquareInternal(visibleNodes,
                            {static_cast<float>(maxWidth), static_cast<float>(totalHeight * i / 100)},
                            {static_cast<float>(gapX), static_cast<float>(gapY)})) {
      LayoutRect rect = boundingRect(visibleNodes);

      int newDiff = static_cast<int>(rect.width() * rect.height() + (rect.width() - rect.height()) * (rect.width() - rect.height()) / 4);
      if(maxWidth >= 0) {
        newDiff = static_cast<int>(rect.height());
      }

      if(diff < 0 || newDiff <= diff) {
        diff = newDiff;
        cols = i;
      }
    }
  }

  layoutSquareInternal(visibleNodes,
                       {static_cast<float>(maxWidth), static_cast<float>(totalHeight * cols / 100)},
                       {static_cast<float>(gapX), static_cast<float>(gapY)});
}

bool ListLayouter::layoutSquareInternal(std::vector<std::shared_ptr<DummyNode>>& visibleNodes,
                                        const Vec2f& maxSize,
                                        const Vec2f& gap) {
  int x = 0;
  int y = 0;

  int width = 0;

  for(std::shared_ptr<DummyNode> node : visibleNodes) {
    node->position.x = static_cast<float>(x);
    node->position.y = static_cast<float>(y);

    y += static_cast<int>(node->size.y + gap.y);
    width = std::max(width, static_cast<int>(node->size.x));

    if(maxSize.x > 0.0f && static_cast<float>(x + width) > maxSize.x) {
      return false;
    }

    if(static_cast<float>(y) >= maxSize.y) {
      y = 0;
      x += static_cast<int>(static_cast<float>(width) + gap.x);

      width = 0;
    }
  }

  return true;
}

void ListLayouter::layoutSkewed(std::vector<std::shared_ptr<DummyNode>>* nodes, int gapX, int gapY, int maxWidth) {
  std::vector<std::shared_ptr<DummyNode>> visibleNodes;
  std::multiset<int> nodeWidths;
  for(auto node : *nodes) {
    if(node->getsLayouted()) {
      visibleNodes.push_back(node);
      nodeWidths.insert(static_cast<int>(node->size.x));
    }
  }

  int nodeWidth = nodeWidths.size() ? *nodeWidths.rbegin() : 0;
  if(nodeWidths.size() > 1) {
    nodeWidth = *nodeWidths.rbegin() / 2 + *(++nodeWidths.rbegin()) / 2;
  }

  int height = 0;
  int width = 0;

  int nodesPerRowStart = std::max(int(std::floor(maxWidth * 2 / (nodeWidth + gapX))), 3);
  for(int nodesPerRow = nodesPerRowStart; nodesPerRow >= 3; nodesPerRow--) {
    height = 0;
    width = (nodeWidth * nodesPerRow + gapX * (nodesPerRow - 1)) / 2;

    int x = 0;
    int rowHeight = 0;
    int nodeCount = 0;
    bool evenRow = true;

    for(size_t i = 0; i < visibleNodes.size(); i++) {
      if(nodeCount + 2 > nodesPerRow) {
        height += rowHeight + gapY;
        rowHeight = 0;

        evenRow = !evenRow;
        nodeCount = evenRow ? 0 : 1;
        x = evenRow ? 0 : ((nodeWidth + gapX) / 2);
      }

      DummyNode* node = visibleNodes[i].get();
      node->position.x = static_cast<float>(x) + (static_cast<float>(nodeWidth) - node->size.x) / 2;
      node->position.y = static_cast<float>(height);

      rowHeight = std::max(rowHeight, static_cast<int>(node->size.y));
      x += nodeWidth + gapX;

      nodeCount += 2;
    }

    height += rowHeight;

    if(height * 3 >= width) {
      break;
    }
  }
}

LayoutRect ListLayouter::boundingRect(const std::vector<std::shared_ptr<DummyNode>>& nodes) {
  LayoutRect rect;

  for(auto node : nodes) {
    if(!node->getsLayouted()) {
      continue;
    }

    if(std::fabs(rect.right - rect.left) < std::numeric_limits<float>::epsilon()) {
      rect.left   = node->position.x;
      rect.top    = node->position.y;
      rect.right  = node->position.x + node->size.x;
      rect.bottom = node->position.y + node->size.y;
    } else {
      rect.left   = static_cast<float>(std::min(static_cast<int>(rect.left),   static_cast<int>(node->position.x)));
      rect.top    = static_cast<float>(std::min(static_cast<int>(rect.top),    static_cast<int>(node->position.y)));
      rect.right  = static_cast<float>(std::max(static_cast<int>(rect.right),  static_cast<int>(node->position.x + node->size.x)));
      rect.bottom = static_cast<float>(std::max(static_cast<int>(rect.bottom), static_cast<int>(node->position.y + node->size.y)));
    }
  }

  return rect;
}

Vec2f ListLayouter::offsetNodes(const std::vector<std::shared_ptr<DummyNode>>& nodes, int top, int left) {
  LayoutRect rect = boundingRect(nodes);
  Vec2f offset{.x = static_cast<float>(left) - rect.left, .y = static_cast<float>(top) - rect.top};

  for(auto node : nodes) {
    if(node->getsLayouted()) {
      node->position += offset;
    }
  }

  return {.x = rect.width(), .y = rect.height()};
}

void ListLayouter::layoutSimple(std::vector<std::shared_ptr<DummyNode>>* nodes, int gapX, int gapY, bool horizontal) {
  int y = 0;
  int x = 0;

  for(const std::shared_ptr<DummyNode>& node : *nodes) {
    if(!node->getsLayouted()) {
      continue;
    }

    node->position.x = static_cast<float>(x);
    node->position.y = static_cast<float>(y);

    if(horizontal) {
      x += static_cast<int>(node->size.x + static_cast<float>(gapX));
    } else {
      y += static_cast<int>(node->size.y + static_cast<float>(gapY));
    }
  }
}
