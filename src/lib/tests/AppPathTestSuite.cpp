#include <filesystem>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AppPath.h"

TEST(AppPath, CxxIndexer) {
#ifdef D_WINDOWS
  const auto exec = "sourcetrail_indexer.exe";
#else
  const auto exec = "sourcetrail_indexer";
#endif

  EXPECT_EQ(exec, AppPath::getCxxIndexerFilePath().str());
  EXPECT_FALSE(AppPath::setCxxIndexerDirectoryPath(FilePath{""}));

  EXPECT_FALSE(AppPath::setSharedDataDirectoryPath(FilePath{""}));
  EXPECT_EQ("", AppPath::getSharedDataDirectoryPath().str());
  const auto tempDir = std::filesystem::temp_directory_path();
  EXPECT_TRUE(AppPath::setSharedDataDirectoryPath(FilePath{tempDir / "shared"}));
  EXPECT_EQ(tempDir / "shared", AppPath::getSharedDataDirectoryPath().str());
  EXPECT_EQ(tempDir / "shared" / exec, AppPath::getCxxIndexerFilePath().str());

  EXPECT_TRUE(AppPath::setCxxIndexerDirectoryPath(FilePath{tempDir}));
  EXPECT_EQ(tempDir / exec, AppPath::getCxxIndexerFilePath().str());
}