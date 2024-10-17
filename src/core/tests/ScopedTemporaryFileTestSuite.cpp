#include <filesystem>
#include <fstream>
#include <string>

#include <fmt/format.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <system_error>

#include "ScopedTemporaryFile.hpp"

namespace fs = std::filesystem;
using namespace testing;

struct ScopedTemporaryFileFix : testing::Test {
  void SetUp() override {
    tempPath = fs::temp_directory_path() / ("tempFile" + fmt::format("{}", rand()));
  }

  void TearDown() override {
    std::error_code errorCode;
    if(fs::exists(tempPath, errorCode)) {
      fs::remove(tempPath, errorCode);
    }
  }
  fs::path tempPath;
};

TEST_F(ScopedTemporaryFileFix, goodCaseCreateEmptyFile) {
  std::error_code errorCode;
  {
    auto scopedTemporaryFile = utility::ScopedTemporaryFile::createEmptyFile(tempPath);
    EXPECT_TRUE(fs::exists(tempPath, errorCode));
  }
  EXPECT_FALSE(fs::exists(tempPath, errorCode));
}

TEST_F(ScopedTemporaryFileFix, emptyArgCreateEmptyFile) {
  std::error_code errorCode;
  auto scopedTemporaryFile = utility::ScopedTemporaryFile::createEmptyFile({});
  EXPECT_FALSE(scopedTemporaryFile);
  EXPECT_FALSE(fs::exists(tempPath, errorCode));
}

TEST_F(ScopedTemporaryFileFix, fileExistsCreateEmptyFile) {
  std::error_code errorCode;

  std::ofstream{tempPath}.flush();
  ASSERT_TRUE(fs::exists(tempPath, errorCode));

  auto scopedTemporaryFile = utility::ScopedTemporaryFile::createEmptyFile(tempPath);
  EXPECT_FALSE(scopedTemporaryFile);
  EXPECT_TRUE(fs::exists(tempPath, errorCode));
}

TEST_F(ScopedTemporaryFileFix, invalidPathCreateEmptyFile) {
  std::error_code errorCode;

  auto scopedTemporaryFile = utility::ScopedTemporaryFile::createEmptyFile("/invalid/path");
  EXPECT_FALSE(scopedTemporaryFile);
  EXPECT_FALSE(fs::exists(tempPath, errorCode));
}

TEST_F(ScopedTemporaryFileFix, goodCaseCreateFile) {
  std::error_code errorCode;
  {
    auto scopedTemporaryFile = utility::ScopedTemporaryFile::createFile(tempPath, "Something");
    ASSERT_TRUE(fs::exists(tempPath, errorCode));
    std::ifstream iStream{tempPath};
    ASSERT_TRUE(iStream.is_open());
    std::string buffer;
    const auto fileSize = fs::file_size(tempPath, errorCode);
    ASSERT_FALSE(errorCode);
    buffer.resize(fileSize);
    iStream.read(buffer.data(), static_cast<long>(fileSize));
    EXPECT_THAT(buffer, StrEq("Something"));
  }
  EXPECT_FALSE(fs::exists(tempPath, errorCode));
}
