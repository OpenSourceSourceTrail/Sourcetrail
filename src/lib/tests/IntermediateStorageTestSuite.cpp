#include "IntermediateStorage.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace {

TEST(IntermediateStorageFix, addNodeForFirstTime) {
  // Given:
  IntermediateStorage storage;
  // When:
  StorageNodeData nodeData{};
  const auto result = storage.addNode(nodeData);
  // Then:
  ASSERT_TRUE(result.second);
  ASSERT_EQ(1, result.second);
}

}