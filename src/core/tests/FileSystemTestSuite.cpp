#include <filesystem>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <system_error>

#include "FileSystem.hpp"

namespace {

constexpr const char* const ExistingFilePath = "data/FileSystemTestSuite/main.cpp";
constexpr std::uintmax_t ExistingFileSize = 126U;
constexpr const char* const MissingFilePath = "missing.txt";

namespace fs = std::filesystem;

struct FileSystemCreateDirectoryFix : testing::Test {
  void SetUp() override {
    std::error_code errorCode;
    ASSERT_FALSE(fs::exists(mExistingDirectory, errorCode) && fs::is_directory(mExistingDirectory, errorCode));
  }

  void TearDown() override {
    if(std::error_code errorCode; fs::exists(mExistingDirectory, errorCode) && fs::is_directory(mExistingDirectory, errorCode)) {
      std::ignore = fs::remove_all(mExistingDirectory, errorCode);
    }
  }

  fs::path mExistingDirectory = fs::temp_directory_path() / "existing";
  core::utility::filesystem::FileSystem mFileSystem;
};

TEST_F(FileSystemCreateDirectoryFix, goodCase) {
  const auto result = mFileSystem.create_directory(mExistingDirectory);
  EXPECT_TRUE(result);
}

TEST_F(FileSystemCreateDirectoryFix, directoryExists) {
  const auto result = mFileSystem.create_directory(fs::path{ExistingFilePath}.parent_path());
  ASSERT_FALSE(result);
  EXPECT_THAT(result.error(), testing::StartsWith("Failed to create directory"));
}

TEST_F(FileSystemCreateDirectoryFix, goodCaseCopyAttributes) {
  const auto result = mFileSystem.create_directory(mExistingDirectory, fs::path{ExistingFilePath}.parent_path());
  EXPECT_TRUE(result);
}

TEST_F(FileSystemCreateDirectoryFix, failDirectoryExists) {
  const auto result = mFileSystem.create_directory(fs::path{ExistingFilePath}.parent_path(), mExistingDirectory);
  ASSERT_FALSE(result);
  EXPECT_THAT(result.error(), testing::StartsWith("Failed to create directory"));
}

TEST(FileSystemCurrentPath, goodCase) {
  const auto result = core::utility::filesystem::FileSystem{}.current_path();
  ASSERT_TRUE(result);
  EXPECT_THAT(*result, fs::current_path());
}

struct FileSystemSetCurrentPathFix : testing::Test {
  void SetUp() override {
    std::error_code errorCode;
    mCurrentPath = fs::current_path(errorCode);
    ASSERT_FALSE(errorCode);
  }
  void TearDown() override {
    std::error_code errorCode;
    fs::current_path(mCurrentPath, errorCode);
  }

  fs::path mCurrentPath;
  core::utility::filesystem::FileSystem mFileSystem;
};

TEST_F(FileSystemSetCurrentPathFix, goodCase) {
  const auto result = mFileSystem.current_path(fs::temp_directory_path());
  EXPECT_TRUE(result);
}

TEST_F(FileSystemSetCurrentPathFix, failed) {
  const auto result = mFileSystem.current_path(ExistingFilePath);
  ASSERT_FALSE(result);
  EXPECT_THAT(result.error(), testing::StartsWith("Failed to set current path"));
}

struct FileSystemFix : testing::Test {
  void SetUp() override {
    std::error_code errorCode;
    ASSERT_TRUE(fs::exists(ExistingFilePath, errorCode));
    ASSERT_FALSE(fs::exists(MissingFilePath, errorCode));
  }
  core::utility::filesystem::FileSystem mFileSystem;
};

TEST_F(FileSystemFix, exists_goodCase) {
  const auto fileExists = mFileSystem.exists(ExistingFilePath);
  ASSERT_TRUE(fileExists);
}

TEST_F(FileSystemFix, exists_missingFile) {
  const auto fileExists = mFileSystem.exists(MissingFilePath);
  ASSERT_FALSE(fileExists);
  EXPECT_THAT(fileExists.error(), testing::StartsWith("File does not exist"));
}

TEST_F(FileSystemFix, fileSize_goodCase) {
  const auto fileSize = mFileSystem.file_size(ExistingFilePath);
  ASSERT_TRUE(fileSize);
  EXPECT_EQ(ExistingFileSize, *fileSize);
}

TEST_F(FileSystemFix, fileSize_missingFile) {
  const auto fileSize = mFileSystem.file_size(MissingFilePath);
  ASSERT_FALSE(fileSize);
}

TEST_F(FileSystemFix, remove_goodCase) {
  auto tempFile = fs::path{ExistingFilePath}.parent_path() / "temp.cpp";
  std::filesystem::copy(ExistingFilePath, tempFile);
  const auto fileSize = mFileSystem.remove(tempFile);
  ASSERT_TRUE(fileSize);
  if(std::error_code errorCode; fs::exists(tempFile, errorCode)) {
    ASSERT_TRUE(fs::remove(tempFile, errorCode));
  }
}

TEST_F(FileSystemFix, remove_missingFile) {
  const auto fileSize = mFileSystem.remove(MissingFilePath);
  ASSERT_FALSE(fileSize);
}

TEST_F(FileSystemFix, remove_all_goodCase) {
  fs::path tmp{std::filesystem::temp_directory_path()};
  std::filesystem::create_directories(tmp / "abcdef/example");
  const auto fileSize = mFileSystem.remove_all(tmp / "abcdef");
  ASSERT_TRUE(fileSize);
  EXPECT_EQ(2U, *fileSize);
  if(std::error_code errorCode; fs::exists(tmp / "abcdef", errorCode)) {
    std::ignore = fs::remove(tmp / "abcdef", errorCode);
  }
}

TEST_F(FileSystemFix, remove_all_missingFile) {
  const auto fileSize = mFileSystem.remove_all(MissingFilePath);
  ASSERT_TRUE(fileSize);
  EXPECT_EQ(0U, *fileSize);
}

TEST_F(FileSystemFix, rename_goodCase) {
  auto tempFile = fs::path{ExistingFilePath}.parent_path() / "temp.cpp";
  fs::copy_file(ExistingFilePath, tempFile);
  ASSERT_TRUE(fs::exists(tempFile));
  auto newFile = fs::path{ExistingFilePath}.parent_path() / "new.cpp";

  const auto result = mFileSystem.rename(tempFile, newFile);
  ASSERT_TRUE(result);
  if(std::error_code errorCode; fs::exists(newFile, errorCode)) {
    std::ignore = fs::remove(newFile, errorCode);
  }
}

TEST_F(FileSystemFix, rename_missingFile) {
  const auto result = mFileSystem.rename(MissingFilePath, "/");
  ASSERT_FALSE(result);
}

}    // namespace
