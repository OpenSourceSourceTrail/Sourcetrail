#include "utility.h"

#include <range/v3/to_container.hpp>
#include <range/v3/view/transform.hpp>

size_t utility::digits(size_t n) {
  int digits = 1;

  while(n >= 10) {
    n /= 10;
    digits++;
  }

  return digits;
}

namespace utility {
std::vector<std::filesystem::path> toStlPath(const std::vector<FilePath>& oldPaths) {
  return oldPaths |
      ranges::cpp20::views::transform([](const FilePath& file) -> std::filesystem::path { return file.wstr(); }) |
      ranges::to<std::vector>();
}

std::vector<FilePath> toFilePath(const std::vector<std::filesystem::path>& oldPaths) {
  return oldPaths | ranges::cpp20::views::transform([](const std::filesystem::path& file) -> FilePath {
           return FilePath {file.wstring()};
         }) |
      ranges::to<std::vector>();
}
}    // namespace utility