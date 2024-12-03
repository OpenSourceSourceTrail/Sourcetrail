#include <gtest/gtest.h>

#include "utility.h"

TEST(utility, trimBlankSpacesOfString) {
  EXPECT_TRUE(utility::trim(" foo  ") == "foo");
}

TEST(utility, trimBlankSpacesOfWstring) {
  EXPECT_TRUE(utility::trim(L" foo  ") == L"foo");
}
