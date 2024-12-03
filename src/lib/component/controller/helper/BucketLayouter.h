#pragma once
// STL
#include <map>

#include <QVector2D>

// internal
#include "DummyNode.h"
#include "GlobalId.hpp"

struct DummyEdge;

class Bucket {
public:
  Bucket();
  Bucket(int i_, int j_);

  int getWidth() const;
  int getHeight() const;

  bool hasNode(std::shared_ptr<DummyNode> node) const;
  void addNode(std::shared_ptr<DummyNode> node);
  const DummyNode::BundledNodesSet& getNodes() const;

  void preLayout(QVector2D viewSize, bool addVerticalSplit, bool forceVerticalSplit);
  void layout(int x, int y, int width, int height);

  const std::vector<int> getColWidths() const;
  int getMiddleGapX() const;

  int i = 0;
  int j = 0;

private:
  int m_width = 0;
  int m_height = 0;

  DummyNode::BundledNodesSet m_nodes;

  std::vector<int> m_colWidths;
};


class BucketLayouter {
public:
  BucketLayouter(QVector2D viewSize);
  void createBuckets(std::vector<std::shared_ptr<DummyNode>>& nodes, const std::vector<std::shared_ptr<DummyEdge>>& edges);
  void layoutBuckets(bool addVerticalSplit);

  std::vector<std::shared_ptr<DummyNode>> getSortedNodes();

private:
  std::shared_ptr<DummyNode> findTopMostDummyNodeRecursive(std::vector<std::shared_ptr<DummyNode>>& nodes,
                                                           Id tokenId,
                                                           std::shared_ptr<DummyNode> top);

  Bucket* getBucket(int i, int j);
  Bucket* getBucket(std::shared_ptr<DummyNode> node);

  QVector2D m_viewSize;
  std::map<int, std::map<int, Bucket>> m_buckets;

  int m_i1;
  int m_j1;
  int m_i2;
  int m_j2;

  DummyNode* m_activeParentNode = nullptr;
};
