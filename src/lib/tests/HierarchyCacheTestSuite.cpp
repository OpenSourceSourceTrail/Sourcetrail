#include <gtest/gtest.h>

#include "HierarchyCache.h"
#include "utility.h"

class TestEdge {
public:
  TestEdge(Id sourceId, Id targetId, const std::vector<Id>& edgeIds)
      : m_sourceId(sourceId), m_targetId(targetId), m_edgeIds(edgeIds) {
    std::sort(m_edgeIds.begin(), m_edgeIds.end());
  }

  TestEdge(std::tuple<Id, Id, std::vector<size_t>> tuple)
      : m_sourceId(std::get<0>(tuple)), m_targetId(std::get<1>(tuple)), m_edgeIds(std::get<2>(tuple)) {
    std::sort(m_edgeIds.begin(), m_edgeIds.end());
  }

  std::string toString() const {
    std::string ret = "s:" + std::to_string(m_sourceId) + ";";
    ret += "t:" + std::to_string(m_targetId) + ";";
    for(Id edgeId : m_edgeIds) {
      ret += "e:" + std::to_string(edgeId) + ";";
    }
    return ret;
  }

  const Id m_sourceId;
  const Id m_targetId;
  std::vector<Id> m_edgeIds;
};

std::vector<std::string> getSerializedInheritanceEdges(HierarchyCache& cache, Id nodeId, std::set<Id> nodeIds) {
  std::vector<std::string> inheritanceEdges;
  for(const std::tuple<Id, Id, std::vector<size_t>>& edge : cache.getInheritanceEdgesForNodeId(nodeId, nodeIds)) {
    inheritanceEdges.push_back(TestEdge(edge).toString());
  }
  return inheritanceEdges;
}

TEST(HierarchyCache, returnsNoInheritanceEdgesForEmptyNodeSet) {
  HierarchyCache cache;
  cache.createInheritance(1, 1, 2);
  std::vector<std::string> inheritanceEdges = getSerializedInheritanceEdges(cache, 1, {});
  EXPECT_TRUE(inheritanceEdges.size() == 0);
}

TEST(HierarchyCache, returnsDirectInheritanceEdgeForNonEmptyNodeSet) {
  HierarchyCache cache;
  cache.createInheritance(1, 1, 2);
  std::vector<std::string> inheritanceEdges = getSerializedInheritanceEdges(cache, 1, {2});
  EXPECT_TRUE(inheritanceEdges.size() == 1);
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 2, {1}).toString()));
}

TEST(HierarchyCache, returnsMultipleIndirectInheritanceEdgesForNonEmptyNodeSet) {
  HierarchyCache cache;
  cache.createInheritance(1, 1, 2);
  cache.createInheritance(2, 2, 3);
  std::vector<std::string> inheritanceEdges = getSerializedInheritanceEdges(cache, 1, {1, 2, 3});
  EXPECT_TRUE(inheritanceEdges.size() == 2);
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 2, {1}).toString()));
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 3, {1, 2}).toString()));
}

TEST(HierarchyCache, skipsInheritanceForNodesNotInSet) {
  HierarchyCache cache;
  cache.createInheritance(1, 1, 2);
  cache.createInheritance(2, 2, 3);
  std::vector<std::string> inheritanceEdges = getSerializedInheritanceEdges(cache, 1, {3});
  EXPECT_TRUE(inheritanceEdges.size() == 1);
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 3, {1, 2}).toString()));
}

TEST(HierarchyCache, returnsInheritanceEdgesForCyclicInheritance) {
  HierarchyCache cache;
  cache.createInheritance(1, 1, 2);
  cache.createInheritance(2, 2, 1);
  std::vector<std::string> inheritanceEdges = getSerializedInheritanceEdges(cache, 1, {1, 2});
  EXPECT_TRUE(inheritanceEdges.size() == 2);
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 2, {1, 2}).toString()));
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 1, {1, 2}).toString()));
}

TEST(HierarchyCache, returnsInheritanceEdgesForDiamondInheritance) {
  HierarchyCache cache;
  cache.createInheritance(1, 1, 2);
  cache.createInheritance(2, 1, 3);
  cache.createInheritance(3, 2, 4);
  cache.createInheritance(4, 3, 4);
  std::vector<std::string> inheritanceEdges = getSerializedInheritanceEdges(cache, 1, {1, 2, 3, 4});
  EXPECT_TRUE(inheritanceEdges.size() == 3);
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 2, {1}).toString()));
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 3, {2}).toString()));
  EXPECT_TRUE(utility::containsElement(inheritanceEdges, TestEdge(1, 4, {1, 2, 3, 4}).toString()));
}
