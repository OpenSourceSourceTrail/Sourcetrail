#include <gtest/gtest.h>

#include "utilityApp.h"

TEST(utilityAppTestSuite, searchPath) {
#ifdef _WIN32
  const std::filesystem::path exec = "cmd.exe";
  const std::filesystem::path full_exec = "C:/Windows/system32/cmd.exe";
#else
  const std::filesystem::path exec = "bash";
  const std::filesystem::path full_exec = "/usr/bin/bash";
#endif
  EXPECT_EQ(utility::searchPath(exec), full_exec);
}

TEST(utilityAppTestSuite, emptyPath) {
#ifdef _WIN32
  const std::filesystem::path exec = "";
  const std::filesystem::path full_exec = "";
#else
  const std::filesystem::path exec = "";
  const std::filesystem::path full_exec = "";
#endif
  EXPECT_EQ(utility::searchPath(exec), full_exec);
}

TEST(utilityAppTestSuite, getOsType) {
#if defined(_WIN32)
  EXPECT_EQ(utility::getOsType(), OsType::Windows);
#elif defined(__APPLE__)
  EXPECT_EQ(utility::getOsType(), OsType::MacOS);
#elif defined(__linux__)
  EXPECT_EQ(utility::getOsType(), OsType::Linux);
#endif
}