#include "utility.h"

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

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
  return oldPaths | ranges::cpp20::views::transform([](const FilePath& file) -> std::filesystem::path { return file.wstr(); }) |
      ranges::to<std::vector>();
}

std::vector<FilePath> toFilePath(const std::vector<std::filesystem::path>& oldPaths) {
  return oldPaths |
      ranges::cpp20::views::transform([](const std::filesystem::path& file) -> FilePath { return FilePath{file.wstring()}; }) |
      ranges::to<std::vector>();
}
}    // namespace utility