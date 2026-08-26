#pragma once
#include <vector>

#include "FilePath.h"
#include "ICxxToolchain.h"

namespace utility {

/** The include paths a set of compile commands mentions, split by the flag that introduced them. */
class CompilationDatabase {
public:
  explicit CompilationDatabase(const std::vector<CxxCompileCommand>& commands);
  /** Reads the database at `cdbPath` through the registered toolchain; empty when there is none. */
  explicit CompilationDatabase(const FilePath& cdbPath);

  [[nodiscard]] std::vector<FilePath> getAllHeaderPaths() const;
  [[nodiscard]] std::vector<FilePath> getHeaderPaths() const;
  [[nodiscard]] std::vector<FilePath> getSystemHeaderPaths() const;
  [[nodiscard]] std::vector<FilePath> getFrameworkHeaderPaths() const;

private:
  std::vector<FilePath> mHeaders;
  std::vector<FilePath> mSystemHeaders;
  std::vector<FilePath> mFrameworkHeaders;
};

}    // namespace utility
