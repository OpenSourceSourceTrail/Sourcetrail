#include <algorithm>
#include <any>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "FileSystem.h"
#include "utility.h"

namespace fs = std::filesystem;

namespace {
[[maybe_unused]] bool isInFiles(const std::set<FilePath>& files, const FilePath& filename) {
  return std::end(files) != files.find(filename);
}

[[maybe_unused]] bool isInFileInfos(const std::vector<FileInfo>& infos, const std::wstring& filename) {
  return std::any_of(std::cbegin(infos), std::cend(infos), [wFileName = FilePath(filename).getCanonical().wstr()](const auto& info) {
    return info.path.getAbsolute().wstr() == wFileName;
  });
}

[[maybe_unused]] bool isInFileInfos(const std::vector<FileInfo>& infos, const std::wstring& filename, const std::wstring& filename2) {
  return std::any_of(std::cbegin(infos),
                     std::cend(infos),
                     [wFileName = FilePath(filename).getCanonical().wstr(),
                      wFileName2 = FilePath(filename2).getCanonical().wstr()](const auto& info) {
                       const auto wPath = info.path.wstr();
                       return wPath == wFileName || wPath == wFileName2;
                     });
}
}    // namespace

TEST(FileSystem, findCppFiles) {
  std::vector<std::wstring> cppFiles = utility::convert<FilePath, std::wstring>(
      file::getFilePathsFromDirectory(FilePath(L"data/FileSystemTestSuite"), {L".cpp"}),
      [](const FilePath& filePath) { return filePath.wstr(); });

  EXPECT_TRUE(cppFiles.size() == 4);
  EXPECT_TRUE(utility::containsElement<std::wstring>(cppFiles, L"data/FileSystemTestSuite/main.cpp"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(cppFiles, L"data/FileSystemTestSuite/Settings/sample.cpp"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(cppFiles, L"data/FileSystemTestSuite/src/main.cpp"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(cppFiles, L"data/FileSystemTestSuite/src/test.cpp"));
}

TEST(FileSystem, findHFiles) {
  std::vector<std::wstring> headerFiles = utility::convert<FilePath, std::wstring>(
      file::getFilePathsFromDirectory(FilePath(L"data/FileSystemTestSuite"), {L".h"}),
      [](const FilePath& filePath) { return filePath.wstr(); });

  EXPECT_TRUE(headerFiles.size() == 3);
  EXPECT_TRUE(utility::containsElement<std::wstring>(headerFiles, L"data/FileSystemTestSuite/tictactoe.h"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(headerFiles, L"data/FileSystemTestSuite/Settings/player.h"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(headerFiles, L"data/FileSystemTestSuite/src/test.h"));
}

TEST(FileSystem, findAllSourceFiles) {
  std::vector<std::wstring> sourceFiles = utility::convert<FilePath, std::wstring>(
      file::getFilePathsFromDirectory(FilePath(L"data/FileSystemTestSuite"), {L".h", L".hpp", L".cpp"}),
      [](const FilePath& filePath) { return filePath.wstr(); });

  EXPECT_TRUE(sourceFiles.size() == 8);
}

// TODO(Hussein): Fix next test
#define DISABLED 0
#if DISABLED
TEST(FileSystem, findFileInfos) {
#  ifndef _WIN32
  std::vector<FilePath> directoryPaths;
  directoryPaths.emplace_back(L"./data/FileSystemTestSuite/src");

  std::vector<FileInfo> files = file::getFileInfosFromPaths(directoryPaths, {L".h", L".hpp", L".cpp"}, false);

  EXPECT_TRUE(files.size() == 3);
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/main.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.h"));
#  endif
}

TEST(FileSystem, findFileInfosWithSymlinks) {
#  ifndef _WIN32
  std::vector<FilePath> directoryPaths;
  directoryPaths.emplace_back(L"./data/FileSystemTestSuite/src");

  const auto files = file::getFileInfosFromPaths(directoryPaths, {L".h", L".hpp", L".cpp"}, true);

  EXPECT_TRUE(files.size() == 5);
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/Settings/player.h", L"./data/FileSystemTestSuite/player.h"));
  EXPECT_TRUE(
      isInFileInfos(files, L"./data/FileSystemTestSuite/src/Settings/sample.cpp", L"./data/FileSystemTestSuite/sample.cpp"));
  EXPECT_TRUE(
      isInFileInfos(files, L"./data/FileSystemTestSuite/src/main.cpp", L"./data/FileSystemTestSuite/src/Settings/src/main.cpp"));
  EXPECT_TRUE(
      isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.cpp", L"./data/FileSystemTestSuite/src/Settings/src/test.cpp"));
  EXPECT_TRUE(
      isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.h", L"./data/FileSystemTestSuite/src/Settings/src/test.h"));
#  endif
}
#endif

// TODO(Hussein): Fix next test
#if DISABLED
TEST(FileSystem, findSymlinkedDirectories) {
#  ifndef _WIN32
  std::vector<FilePath> directoryPaths;
  directoryPaths.emplace_back("./data/FileSystemTestSuite/src");

  const auto dirs = file::getSymLinkedDirectories(directoryPaths);

  EXPECT_TRUE(dirs.size() == 2);
#  endif
}
#endif

TEST(FileSystem, isPortableFileName) {
  // Given: valid filename
  const std::string validFileName = "filesystem.cpp";

  // When: calling isPortableFileName
  auto result = file::isPortableFileName(validFileName);

  // Then: result equals to true
  EXPECT_TRUE(result);

  // Given invalid filename
  const std::string invalidFileName = "filesystem.c++";

  // When: calling isPortableFileName
  result = file::isPortableFileName(invalidFileName);

  // Then: result equals to false
  EXPECT_FALSE(result);
}

TEST(FileSystem, renameFile) {
  // Give: create tmp file
  auto tmpFilePath = fs::temp_directory_path() / "tmp.txt";
  { std::ofstream file{tmpFilePath.c_str()}; }

  auto newTmpFilePath = fs::temp_directory_path() / "temp.txt";
  auto unknownFilePath = fs::temp_directory_path() / "tempxxx.txt";

  // When: calling rename
  auto result = file::rename(FilePath{tmpFilePath.string()}, FilePath{newTmpFilePath.string()});

  // Then: result equals to true
  EXPECT_TRUE(result);

  // When: calling rename
  result = file::rename(FilePath{unknownFilePath.string()}, FilePath{tmpFilePath.string()});

  // Then: result equals to false
  EXPECT_FALSE(result);

  // When: calling rename
  result = file::rename(FilePath{newTmpFilePath.string()}, FilePath{newTmpFilePath.string()});

  // Then: result equals to false
  EXPECT_FALSE(result);

  // Clean
  fs::remove(newTmpFilePath);
}

TEST(FileSystem, copyFile) {
  // Give: from path & to path
  auto fromPath = FilePath(LIB_TEST_ROOT_DIR).concatenate(FilePath{"FileSystemTestSuite.cpp"});
  auto toPath = FilePath((fs::temp_directory_path() / "FileSystemTestSuite.cpp").string());
  auto unknownFilePath = FilePath(fs::temp_directory_path() / "tempxxx.txt");

  // When: calling copyFile
  auto result = file::copyFile(fromPath, toPath);

  // Then: result equals to true
  EXPECT_TRUE(result);

  // When: calling copyFile
  result = file::copyFile(unknownFilePath, fromPath);

  // Then: result equals to false
  EXPECT_FALSE(result);

  // When: calling copyFile
  result = file::copyFile(toPath, toPath);

  // Then: result equals to false
  EXPECT_FALSE(result);

  // Clean
  fs::remove(toPath.str());
}
