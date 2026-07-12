#pragma once
#include <vector>

#include "FilePath.h"

namespace utility {

class CompilationDatabase {
public:
  explicit CompilationDatabase(FilePath filePath);

  [[nodiscard]] std::vector<FilePath> getAllHeaderPaths() const;
  [[nodiscard]] std::vector<FilePath> getHeaderPaths() const;
  [[nodiscard]] std::vector<FilePath> getSystemHeaderPaths() const;
  [[nodiscard]] std::vector<FilePath> getFrameworkHeaderPaths() const;

private:
  void init();

  FilePath mFilePath;
  std::vector<FilePath> mHeaders;
  std::vector<FilePath> mSystemHeaders;
  std::vector<FilePath> mFrameworkHeaders;
};

}    // namespace utility