#include <gtest/gtest.h>

#include "utilityApp.h"

TEST(utilityAppTestSuite, searchPath) {
#ifdef D_WINDOWS
  const std::filesystem::path exec = "";
  const std::filesystem::path full_exec = "";
#else
  const std::filesystem::path exec = "bash";
  const std::filesystem::path full_exec = "/usr/bin/bash";
#endif
  EXPECT_EQ(utility::searchPath(exec), full_exec);
}

TEST(utilityAppTestSuite, emptyPath) {
#ifdef D_WINDOWS
  const std::filesystem::path exec = "";
  const std::filesystem::path full_exec = "";
#else
  const std::filesystem::path exec = "";
  const std::filesystem::path full_exec = "";
#endif
  EXPECT_EQ(utility::searchPath(exec), full_exec);
}

TEST(utilityAppTestSuite, getOsType) {
#if defined(D_WINDOWS)
  EXPECT_EQ(utility::getOsType(), OsType::Windows);
#elif defined(D_LINUX)
  EXPECT_EQ(utility::getOsType(), OsType::Linux);
#endif
}