#include <algorithm>
#include <iterator>

#include <range/v3/algorithm/transform.hpp>

#include <gtest/gtest.h>

#include "FileManager.h"
#include "FilePath.h"
#include "FilePathFilter.h"
#include "FileSystem.h"
#include "utility.h"

struct FileManagerFix : testing::Test {
  void SetUp() override {
    filePaths = FileSystem::getFilePathsFromDirectory(FilePath(L"./data/FileManagerTestSuite/src/"));
    ASSERT_EQ(3, filePaths.size());

    sourceExtensions.resize(filePaths.size());
    ranges::cpp20::transform(
        filePaths, sourceExtensions.begin(), [](const auto& filePath) -> std::wstring { return filePath.extension(); });
    ASSERT_EQ(3, sourceExtensions.size());
  }

  std::vector<FilePath> filePaths;
  std::vector<FilePathFilter> excludeFilters;
  std::vector<FilePath> sourcePaths{
      FilePath{L"./data/FileManagerTestSuite/src/"}, FilePath{L"./data/FileManagerTestSuite/include/"}};
  std::vector<std::wstring> sourceExtensions;

  FileManager fileManager;
};

TEST_F(FileManagerFix, hasAddedFilePathsAfterFirstFetch) {
  fileManager.update(sourcePaths, excludeFilters, sourceExtensions);
  const auto foundSourcePaths = utility::toVector(fileManager.getAllSourceFilePaths());

  ASSERT_EQ(3, foundSourcePaths.size());
  EXPECT_TRUE(utility::containsElement<FilePath>(foundSourcePaths, filePaths[0]));
  EXPECT_TRUE(utility::containsElement<FilePath>(foundSourcePaths, filePaths[1]));
  EXPECT_TRUE(utility::containsElement<FilePath>(foundSourcePaths, filePaths[2]));

  ASSERT_EQ(2, fileManager.getSourcePaths().size());
  ASSERT_TRUE(fileManager.hasSourceFilePath(FilePath{L"data/FileManagerTestSuite/src/a.cpp"}.getCanonical()));
}
