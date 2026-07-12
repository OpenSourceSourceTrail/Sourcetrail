// GTest
#include <gtest/gtest.h>
// internal
#include "SingleValueCache.h"

using namespace ::testing;

// NOLINTNEXTLINE
TEST(SingleValueCache, goodCase) {
  int ExpectedValue = 100;
  int count = 0;
  SingleValueCache<int> value([&count, &ExpectedValue]() -> int {
    ++count;
    return ExpectedValue;
  });
  ASSERT_EQ(ExpectedValue, value.getValue());
  ASSERT_EQ(1, count);
  // The value is cache so the calculator won't be called againe.
  ASSERT_EQ(ExpectedValue, value.getValue());
  ASSERT_EQ(1, count);
  // Clear value
  value.clear();
  ExpectedValue = 200;
  ASSERT_EQ(ExpectedValue, value.getValue());
  ASSERT_EQ(2, count);
}