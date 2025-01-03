#include <algorithm>
#include <any>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "FileSystem.h"
#include "ScopedTemporaryFile.hpp"
#include "utility.h"

namespace fs = std::filesystem;

namespace {
[[maybe_unused]] bool isInFileInfos(const std::vector<FileInfo>& infos, const std::wstring& filename) {
  return std::any_of(std::cbegin(infos), std::cend(infos), [wFileName = FilePath(filename).getCanonical().wstr()](const auto& info) {
    return info.path.getAbsolute().wstr() == wFileName;
  });
}
}    // namespace

TEST(FileSystem, findCppFiles) {
  std::vector<std::wstring> cppFiles = utility::convert<FilePath, std::wstring>(
      filesystem::getFilePathsFromDirectory(FilePath(L"data/FileSystemTestSuite"), {L".cpp"}),
      [](const FilePath& filePath) { return filePath.wstr(); });

  EXPECT_TRUE(cppFiles.size() == 4);

  EXPECT_TRUE(utility::containsElement<std::wstring>(cppFiles, L"data/FileSystemTestSuite/main.cpp"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(cppFiles, L"data/FileSystemTestSuite/Settings/sample.cpp"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(cppFiles, L"data/FileSystemTestSuite/src/main.cpp"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(cppFiles, L"data/FileSystemTestSuite/src/test.cpp"));
}

TEST(FileSystem, findHFiles) {
  std::vector<std::wstring> headerFiles = utility::convert<FilePath, std::wstring>(
      filesystem::getFilePathsFromDirectory(FilePath(L"data/FileSystemTestSuite"), {L".h"}),
      [](const FilePath& filePath) { return filePath.wstr(); });

  EXPECT_TRUE(headerFiles.size() == 3);

  EXPECT_TRUE(utility::containsElement<std::wstring>(headerFiles, L"data/FileSystemTestSuite/tictactoe.h"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(headerFiles, L"data/FileSystemTestSuite/Settings/player.h"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(headerFiles, L"data/FileSystemTestSuite/src/test.h"));
}

TEST(FileSystem, findAllFiles) {
  auto sourceFiles = filesystem::getFilePathsFromDirectory(FilePath(L"./data/FileSystemTestSuite"));

  EXPECT_TRUE(sourceFiles.size() == 9);
}

TEST(FileSystem, findFilePathsFailed) {
  auto result = utility::convert<FilePath, std::wstring>(
      filesystem::getFilePathsFromDirectory(FilePath(L"./xxx")), [](const FilePath& filePath) { return filePath.wstr(); });
  EXPECT_EQ(result.size(), 0);
}

TEST(FileSystem, failToGetFileInfo) {
  // Given: Unknown file path
  auto unknownFilePath = FilePath(fs::temp_directory_path() / "tempxxx.txt");

  // When: calling getFileInfoForPath
  auto result = filesystem::getFileInfoForPath(unknownFilePath);

  // Then: Expected default FileInfo
  EXPECT_EQ(result, FileInfo());
}

TEST(FileSystem, findFileInfos) {
  std::vector<FilePath> directoryPaths;
  directoryPaths.emplace_back(L"./data/FileSystemTestSuite/src");

  std::vector<FileInfo> files = filesystem::getFileInfosFromPaths(directoryPaths, {L".h", L".hpp", L".cpp"}, false);

  EXPECT_TRUE(files.size() == 2);
#ifndef _WIN32
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.h"));
#else
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\src\\test.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\src\\test.h"));
#endif
}

TEST(FileSystem, findFileInfosWithoutExtensions) {
  std::vector<FilePath> directoryPaths;
  directoryPaths.emplace_back(L"./data/FileSystemTestSuite");

  std::vector<FileInfo> files = filesystem::getFileInfosFromPaths(directoryPaths, {}, false);

  EXPECT_TRUE(files.size() == 8);
#ifndef _WIN32
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.h"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/Settings/player.h"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/Settings/sample.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/update.c"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/tictactoe.h"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/Sound.hpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/main.cpp"));
#else
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\src\\test.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\src\\test.h"));
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\Settings\\player.h"));
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\Settings\\sample.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\update.c"));
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\tictactoe.h"));
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\Sound.hpp"));
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\main.cpp"));
#endif
}

TEST(FileSystem, findFileInfosFailed) {
  std::vector<FilePath> directoryPaths;
  directoryPaths.emplace_back(L"./xxx");

  std::vector<FileInfo> files = filesystem::getFileInfosFromPaths(directoryPaths, {}, false);

  EXPECT_TRUE(files.size() == 0);
}

TEST(FileSystem, findFileInfosWithEmptyPaths) {
  std::vector<FileInfo> files = filesystem::getFileInfosFromPaths({}, {}, false);

  EXPECT_TRUE(files.size() == 0);
}

TEST(FileSystem, findFileInfo) {
  std::vector<FileInfo> files = filesystem::getFileInfosFromPaths(
      {FilePath{L"./data/FileSystemTestSuite/src/test.cpp"}}, {}, false);

  EXPECT_TRUE(files.size() == 1);
#ifndef _WIN32
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.cpp"));
#else
  EXPECT_TRUE(isInFileInfos(files, L"data\\FileSystemTestSuite\\src\\test.cpp"));
#endif
}

TEST(FileSystem, findFileInfosWithSymlinks) {
#ifndef _WIN32
  std::vector<FilePath> directoryPaths;
  directoryPaths.emplace_back(L"data/FileSystemTestSuite/src");
  const auto files = filesystem::getFileInfosFromPaths(directoryPaths, {L".h", L".hpp", L".cpp"}, true);

  EXPECT_TRUE(files.size() == 5);
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/Settings/player.h"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/Settings/sample.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/main.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.cpp"));
  EXPECT_TRUE(isInFileInfos(files, L"./data/FileSystemTestSuite/src/test.h"));
#endif
}

TEST(FileSystem, findSymlinkedDirectories) {
#ifndef _WIN32
  FilePath directoryPath{L"data/FileSystemTestSuite"};
  auto dirPaths = filesystem::getSymLinkedDirectories(directoryPath);

  EXPECT_TRUE(dirPaths.size() == 2);

  EXPECT_TRUE(std::find(dirPaths.begin(), dirPaths.end(), FilePath(L"data/FileSystemTestSuite/src")) != dirPaths.end());
  EXPECT_TRUE(std::find(dirPaths.begin(), dirPaths.end(), FilePath(L"data/FileSystemTestSuite/Settings")) != dirPaths.end());
#endif
}

TEST(FileSystem, findSymlinkedDirectoriesFailed) {
  const auto dirs = filesystem::getSymLinkedDirectories(FilePath{L"./xxx"});

  EXPECT_TRUE(dirs.size() == 0);
}

TEST(FileSystem, getFileByteSize) {
  // Given: filepath
  const auto filePath = fs::temp_directory_path() / "tmp.txt";
  constexpr size_t size = 2 * 1024 * 1024;      // 2 MB
  const std::vector<char> buffer(size, ' ');    // Fill with ' ' for demonstration
  std::string_view data(buffer.data(), buffer.size());
  const auto generatedFile = utility::ScopedTemporaryFile::createFile(filePath, data);

  // When: calling getFileByteSize
  auto result = filesystem::getFileByteSize(FilePath{filePath});

  // Then: expected out is 2 MB
  EXPECT_EQ(result, size);
}

TEST(FileSystem, failToGetLastWriteTime) {
  // Given: Unknown file path
  auto unknownFilePath = FilePath(fs::temp_directory_path() / "tempxxx.txt");

  // When: calling getLastWriteTime
  auto result = filesystem::getLastWriteTime(unknownFilePath);

  // Then: Expected default TimeStamp
  EXPECT_EQ(result, TimeStamp{boost::posix_time::ptime{}});
}

TEST(FileSystem, renameFile) {
  // Give: create tmp file
  auto tmpFilePath = fs::temp_directory_path() / "tmp.txt";
  auto emptyFile = utility::ScopedTemporaryFile::createEmptyFile(tmpFilePath.string());

  auto newTmpFilePath = fs::temp_directory_path() / "temp.txt";
  auto unknownFilePath = fs::temp_directory_path() / "tempxxx.txt";

  // When: calling rename
  auto result = filesystem::rename(FilePath{tmpFilePath.string()}, FilePath{newTmpFilePath.string()});

  // Then: result equals to true
  EXPECT_TRUE(result);

  // Clean step
  EXPECT_TRUE(filesystem::remove(FilePath{newTmpFilePath.string()}));

  // When: calling rename
  result = filesystem::rename(FilePath{unknownFilePath.string()}, FilePath{tmpFilePath.string()});

  // Then: result equals to false
  EXPECT_FALSE(result);

  // When: calling rename
  result = filesystem::rename(FilePath{newTmpFilePath.string()}, FilePath{newTmpFilePath.string()});

  // Then: result equals to false
  EXPECT_FALSE(result);
}

TEST(FileSystem, copyFile) {
  // Give: from path & to path & Unknown file path
  auto fromPath = FilePath(LIB_TEST_ROOT_DIR).concatenate(FilePath{"FileSystemTestSuite.cpp"});
  auto toPath = FilePath((fs::temp_directory_path() / "FileSystemTestSuite.cpp").string());
  auto unknownFilePath = FilePath(fs::temp_directory_path() / "tempxxx.txt");

  // When: calling copyFile
  auto result = filesystem::copyFile(fromPath, toPath);

  // Then: result equals to true
  EXPECT_TRUE(result);

  // When: calling copyFile
  result = filesystem::copyFile(unknownFilePath, fromPath);

  // Then: result equals to false
  EXPECT_FALSE(result);

  // When: calling copyFile
  result = filesystem::copyFile(toPath, toPath);

  // Then: result equals to false
  EXPECT_FALSE(result);

  // Clean
  fs::remove(toPath.str());
}

TEST(FileSystem, isPortableFileName) {
  // Given: valid filename
  const std::string validFileName = "filesystem.cpp";

  // When: calling isPortableFileName
  auto result = filesystem::isPortableFileName(validFileName);

  // Then: result equals to true
  EXPECT_TRUE(result);

  // Given invalid filename
  const std::string invalidFileName = "filesystem.c++";

  // When: calling isPortableFileName
  result = filesystem::isPortableFileName(invalidFileName);

  // Then: result equals to false
  EXPECT_FALSE(result);
}
