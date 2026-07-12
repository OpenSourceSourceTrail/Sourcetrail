#include <gtest/gtest.h>

#include "ScopedFunctor.h"

TEST(ScopedFunctor, goodCase) {
  bool called = false;
  auto func = [&called]() { called = true; };
  { const ScopedFunctor ScopedFunctor(func); }
  EXPECT_TRUE(called);
}
