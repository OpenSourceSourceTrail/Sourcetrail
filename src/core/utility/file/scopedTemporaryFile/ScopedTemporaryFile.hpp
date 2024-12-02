#pragma once
#include <filesystem>
#include <memory>

namespace utility {

struct ScopedTemporaryFile final {
  using Ptr = std::unique_ptr<ScopedTemporaryFile>;

  static Ptr createEmptyFile(std::filesystem::path filePath);
  static Ptr createFile(std::filesystem::path filePath, std::string_view data);

  ~ScopedTemporaryFile();

  ScopedTemporaryFile(const ScopedTemporaryFile&) = delete;
  ScopedTemporaryFile& operator=(const ScopedTemporaryFile&) = delete;
  ScopedTemporaryFile(ScopedTemporaryFile&&) = delete;
  ScopedTemporaryFile& operator=(ScopedTemporaryFile&&) = delete;

  [[nodiscard]] std::filesystem::path getFilePath() const {
    return mFilePath;
  }

private:
  ScopedTemporaryFile();

  std::filesystem::path mFilePath;
};

}    // namespace utility