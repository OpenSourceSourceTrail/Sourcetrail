#include <filesystem>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "UserPaths.h"

namespace fs = std::filesystem;

TEST(UserPaths, goodCase) {
  const auto tempDir = fs::path{fs::temp_directory_path(), fs::path::path::generic_format};
  EXPECT_TRUE(UserPaths::getUserDataDirectoryPath().empty());
  UserPaths::setUserDataDirectoryPath(FilePath{tempDir});
  EXPECT_EQ(tempDir, UserPaths::getUserDataDirectoryPath().str());

  EXPECT_EQ(tempDir / "ApplicationSettings.xml", UserPaths::getAppSettingsFilePath().str());
  EXPECT_EQ(tempDir / "window_settings.ini", UserPaths::getWindowSettingsFilePath().str());
  EXPECT_EQ(tempDir / "log/", UserPaths::getLogDirectoryPath().str());
}