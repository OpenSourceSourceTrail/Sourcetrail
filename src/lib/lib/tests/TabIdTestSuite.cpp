#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "TabId.h"

// NOLINTNEXTLINE
TEST(TabId, goodCase) {
  EXPECT_EQ(1, TabId::app());
  EXPECT_EQ(2, TabId::background());
  EXPECT_EQ(3, TabId::ignore());

  EXPECT_EQ(0, TabId::currentTab());
  TabId::setCurrentTabId(100);
  EXPECT_EQ(100, TabId::currentTab());

  EXPECT_EQ(10, TabId::nextTab());
  EXPECT_EQ(11, TabId::nextTab());
  EXPECT_EQ(12, TabId::nextTab());
}