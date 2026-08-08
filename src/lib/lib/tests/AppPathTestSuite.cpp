#include <filesystem>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AppPath.h"

TEST(AppPath, sharedDataDirectoryRootsThePluginDirectory) {
  EXPECT_FALSE(AppPath::setSharedDataDirectoryPath(FilePath{""}));
  EXPECT_EQ("", AppPath::getSharedDataDirectoryPath().str());

  const auto tempDir = std::filesystem::temp_directory_path();
  EXPECT_TRUE(AppPath::setSharedDataDirectoryPath(FilePath{tempDir / "shared"}));
  EXPECT_EQ(tempDir / "shared", AppPath::getSharedDataDirectoryPath().str());

  // Every indexer, built-in included, is found by scanning this directory for manifests.
  EXPECT_EQ(tempDir / "shared" / "plugins", AppPath::getPluginsDirectoryPath().str());
}
