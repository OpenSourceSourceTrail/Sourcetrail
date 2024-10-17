// GTest
#include <gmock/gmock.h>
#include <gtest/gtest.h>
// internal
#include "Tree.h"

using namespace ::testing;

// NOLINTNEXTLINE
TEST(Tree, goodCase) {
  const Tree<int> tree{};
  ASSERT_EQ(0, tree.data);

  const Tree<int> tree0{100};
  ASSERT_EQ(100, tree0.data);
}