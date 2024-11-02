#ifndef LIST_LAYOUTER_H
#define LIST_LAYOUTER_H

#include <memory>
#include <vector>

#include <QVector2D>
#include <QVector4D>

struct DummyNode;

class ListLayouter {
public:
  static void layoutRow(std::vector<std::shared_ptr<DummyNode>>* nodes, int gap);
  static void layoutColumn(std::vector<std::shared_ptr<DummyNode>>* nodes, int gap);

  static void layoutMultiColumn(QVector2D viewSize, std::vector<std::shared_ptr<DummyNode>>* nodes);
  static void layoutSquare(std::vector<std::shared_ptr<DummyNode>>* nodes, int maxWidth);
  static void layoutSkewed(std::vector<std::shared_ptr<DummyNode>>* nodes, int gapX, int gapY, int maxWidth);

  static QVector4D boundingRect(const std::vector<std::shared_ptr<DummyNode>>& nodes);
  static QVector2D offsetNodes(const std::vector<std::shared_ptr<DummyNode>>& nodes, int top, int left);

private:
  static void layoutSimple(std::vector<std::shared_ptr<DummyNode>>* nodes, int gapX, int gapY, bool horizontal);
  static bool layoutSquareInternal(std::vector<std::shared_ptr<DummyNode>>& visibleNodes,
                                   const QVector2D& maxSize,
                                   const QVector2D& gap);
};

#endif    // LIST_LAYOUTER_H
