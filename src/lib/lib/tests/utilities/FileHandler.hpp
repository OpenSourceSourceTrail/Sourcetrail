#pragma once
#include <filesystem>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

struct FileHandler final {
  using Ptr = std::unique_ptr<FileHandler>;

  static Ptr createEmptyFile(fs::path filepath);

  static Ptr createFileFromBuffer(fs::path filepath, const std::vector<char>& buffer);

  static Ptr createFileWithRandomData(fs::path filepath, size_t bufferSize);

  static std::vector<char> generateRandomBuffer(size_t bufferSize);

  ~FileHandler();

  FileHandler(const FileHandler&) = delete;
  FileHandler& operator=(const FileHandler&) = delete;
  FileHandler(FileHandler&&) = delete;
  FileHandler& operator=(FileHandler&&) = delete;

  [[nodiscard]] fs::path filePath() const {
    return mFilePath;
  }

  [[nodiscard]] size_t fileSize() const {
    return mFileSize;
  }

private:
  FileHandler(fs::path filePath, size_t bufferSize);

  fs::path mFilePath;
  size_t mFileSize;
};
