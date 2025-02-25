#pragma once
#include <cassert>
#include <cstdint>
#include <ostream>
#include <set>

#include "FilePath.h"
#include "to_underlying.hpp"

enum class RefreshMode : std::uint8_t {
  None = 0,                     ///< No refresh needed
  UpdatedFiles,                 ///< Refresh only modified files
  UpdatedAndIncompleteFiles,    ///< Refresh modified and partially indexed files
  AllFiles                      ///< Full refresh of all files
};

inline std::ostream& operator<<(std::ostream& out, const RefreshMode mode) {
  switch(mode) {
  case RefreshMode::None:
    return out << "None";
  case RefreshMode::UpdatedFiles:
    return out << "UpdatedFiles";
  case RefreshMode::UpdatedAndIncompleteFiles:
    return out << "UpdatedAndIncompleteFiles";
  case RefreshMode::AllFiles:
    return out << "AllFiles";
  default:
    assert(false && "Unhandled RefreshMode");
    return out << "Unknown";
  }
}

inline std::wostream& operator<<(std::wostream& out, const RefreshMode mode) {
  switch(mode) {
  case RefreshMode::None:
    return out << L"None";
  case RefreshMode::UpdatedFiles:
    return out << L"UpdatedFiles";
  case RefreshMode::UpdatedAndIncompleteFiles:
    return out << L"UpdatedAndIncompleteFiles";
  case RefreshMode::AllFiles:
    return out << L"AllFiles";
  default:
    assert(false && "Unhandled RefreshMode");
    return out << L"Unknown";
  }
}

constexpr bool operator==(RefreshMode lhs, RefreshMode rhs) noexcept {
  return utility::to_underlying(lhs) == utility::to_underlying(rhs);
}

struct RefreshInfo final {
  std::set<FilePath> filesToIndex;
  std::set<FilePath> filesToClear;
  std::set<FilePath> nonIndexedFilesToClear;

  RefreshMode mode = RefreshMode::None;
  bool shallow = false;
};
