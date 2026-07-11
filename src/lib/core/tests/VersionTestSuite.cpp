#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Version.h"

TEST(Version, goodCase) {
  auto version = Version::fromString("1.2.3");
  ASSERT_TRUE(version.isValid()) << version.toString();
  EXPECT_THAT(version.toString(), testing::StrEq("1.2.3"));
}