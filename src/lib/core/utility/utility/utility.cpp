#include "utility.h"

#include <ranges>

#include "RangesTo.hpp"

size_t utility::digits(size_t n) {
  constexpr int DigitCount = 10;
  int digits = 1;

  while(n >= DigitCount) {
    n /= DigitCount;
    digits++;
  }

  return static_cast<size_t>(digits);
}

namespace utility {
std::vector<std::filesystem::path> toStlPath(const std::vector<FilePath>& oldPaths) {
  return oldPaths | std::views::transform([](const FilePath& file) -> std::filesystem::path { return file.wstr(); }) |
      utility::toContainer<std::vector<std::filesystem::path>>();
}

std::vector<FilePath> toFilePath(const std::vector<std::filesystem::path>& oldPaths) {
  return oldPaths | std::views::transform([](const std::filesystem::path& file) -> FilePath { return FilePath{file.wstring()}; }) |
      utility::toContainer<std::vector<FilePath>>();
}
}    // namespace utility