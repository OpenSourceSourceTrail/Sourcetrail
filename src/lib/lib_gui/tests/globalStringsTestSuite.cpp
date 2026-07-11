#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "globalStrings.h"

using namespace testing;

// NOLINTNEXTLINE(cert-err58-cpp)
TEST(getGithubUrl, goodCase) {
  constexpr auto GITHUB_URL = "github"_g;
  static_assert(std::is_same_v<const char* const, decltype(GITHUB_URL)>);
  EXPECT_THAT(GITHUB_URL, StrEq("https://github.com/OpenSourceSourceTrail/Sourcetrail"));

  constexpr auto documentation_URL = "documentation"_g;
  EXPECT_THAT(documentation_URL, StrEq("https://github.com/OpenSourceSourceTrail/Sourcetrail/blob/main/DOCUMENTATION.md"));
}