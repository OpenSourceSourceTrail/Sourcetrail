#pragma once
#include <limits>
#include <string>

#include "GlobalId.hpp"

struct ErrorInfo {
  static constexpr size_t Invalid = std::numeric_limits<size_t>::max();

  ErrorInfo() = default;

  ErrorInfo(Id id_,
            std::wstring message_,
            std::wstring filePath_,
            size_t lineNumber_,
            size_t columnNumber_,
            std::wstring translationUnit_,
            bool fatal_,
            bool indexed_)
      : id(id_)
      , message(std::move(message_))
      , filePath(std::move(filePath_))
      , lineNumber(lineNumber_)
      , columnNumber(columnNumber_)
      , translationUnit(std::move(translationUnit_))
      , fatal(fatal_)
      , indexed(indexed_) {}

  Id id = 0;
  std::wstring message;
  std::wstring filePath;
  size_t lineNumber = Invalid;
  size_t columnNumber = Invalid;
  std::wstring translationUnit;
  bool fatal = false;
  bool indexed = false;
};
