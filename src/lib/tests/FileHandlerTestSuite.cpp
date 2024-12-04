#include <filesystem>
#include <fstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utilities/FileHandler.hpp"

using namespace ::testing;

// NOLINTNEXTLINE
TEST(FileHandler, goodCaseEmptyFile) {
  std::error_code errorCode;
  constexpr auto TemptyFilePath = "/tmp/emptyFile0";
  {
    auto fileHandler = FileHandler::createEmptyFile(TemptyFilePath);
    ASSERT_TRUE(fileHandler);

    ASSERT_TRUE(fs::exists(TemptyFilePath, errorCode));
    EXPECT_EQ(0, fs::file_size(TemptyFilePath, errorCode));
    EXPECT_EQ(0, fileHandler->fileSize());
    EXPECT_THAT(fileHandler->filePath().string(), StrEq(TemptyFilePath));
  }
  EXPECT_FALSE(fs::exists(TemptyFilePath, errorCode));
}

// NOLINTNEXTLINE
TEST(FileHandler, emptyFileExists) {
  std::error_code errorCode;
  constexpr auto TemptyFilePath = "/tmp/emptyFile1";
  if(fs::exists(TemptyFilePath, errorCode)) {
    fs::remove(TemptyFilePath, errorCode);
  }

  std::ofstream oStream(TemptyFilePath);
  oStream.close();

  auto fileHandler = FileHandler::createEmptyFile(TemptyFilePath);
  ASSERT_FALSE(fileHandler);

  if(fs::exists(TemptyFilePath, errorCode)) {
    fs::remove(TemptyFilePath, errorCode);
  }
}

// NOLINTNEXTLINE
TEST(FileHandler, emptyFileCanNotCreated) {
#ifdef _WIN32
  constexpr auto TemptyFilePath = "\\emptyFile2";
#else
  constexpr auto TemptyFilePath = "/emptyFile2";
#endif
  auto fileHandler = FileHandler::createEmptyFile(TemptyFilePath);
  ASSERT_FALSE(fileHandler);
}

// NOLINTNEXTLINE
TEST(FileHandler, goodCaseFromBuffer) {
  constexpr auto BufferSize = 32U;
  constexpr auto FilePath = "/tmp/fromDataFile0";
  std::error_code errorCode;
  {
    const std::vector<char> buffer(BufferSize, 0);
    auto fileHandler = FileHandler::createFileFromBuffer(FilePath, buffer);
    ASSERT_TRUE(fileHandler);
    EXPECT_TRUE(fs::exists(FilePath, errorCode));
    EXPECT_EQ(BufferSize, fs::file_size(FilePath, errorCode));
  }
  EXPECT_FALSE(fs::exists(FilePath, errorCode));
}

// NOLINTNEXTLINE
TEST(FileHandler, PassZeroToFromBuffer) {
  constexpr auto FilePath = "/tmp/fromDataFile0";
  std::error_code errorCode;
  {
    const std::vector<char> buffer;
    auto fileHandler = FileHandler::createFileFromBuffer(FilePath, buffer);
    ASSERT_TRUE(fileHandler);
    ASSERT_TRUE(fs::exists(FilePath, errorCode));
    ASSERT_EQ(0, fs::file_size(FilePath, errorCode));
  }
  ASSERT_FALSE(fs::exists(FilePath, errorCode));
}

// NOLINTNEXTLINE
TEST(FileHandler, goodCaseRandomData) {
  constexpr auto RandomFilePath = "/tmp/randomDataFile0";
  constexpr auto RandomFileSize = 512U;
  std::error_code errorCode;
  {
    auto fileHandler = FileHandler::createFileWithRandomData(RandomFilePath, RandomFileSize);
    ASSERT_TRUE(fileHandler);
    EXPECT_TRUE(fs::exists(RandomFilePath, errorCode));
    EXPECT_EQ(RandomFileSize, fs::file_size(RandomFilePath, errorCode));
  }
  EXPECT_FALSE(fs::exists(RandomFilePath, errorCode));
}

// NOLINTNEXTLINE
TEST(FileHandler, PassZeroToRandomData) {
  constexpr auto RandomFilePath = "/tmp/randomDataFile1";
  constexpr auto RandomFileSize = 0U;
  std::error_code errorCode;
  {
    auto fileHandler = FileHandler::createFileWithRandomData(RandomFilePath, RandomFileSize);
    ASSERT_TRUE(fileHandler);
    ASSERT_TRUE(fs::exists(RandomFilePath, errorCode));
    EXPECT_EQ(0, fs::file_size(RandomFilePath, errorCode));
  }
  EXPECT_FALSE(fs::exists(RandomFilePath, errorCode));
}

TEST(FileHandler, GenerageRandomData) {
  constexpr auto RandomFileSize = 512U;
  const auto buffer = FileHandler::generateRandomBuffer(RandomFileSize);
  ASSERT_EQ(RandomFileSize, buffer.size());
}
