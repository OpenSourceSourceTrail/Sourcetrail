#pragma once
// STL
#include <string>
// internal
#include "GlobalId.hpp"

struct StorageErrorData {
  StorageErrorData() = default;

  StorageErrorData(std::wstring message_, std::wstring translationUnit_, bool fatal_, bool indexed_)
      : message(std::move(message_)), translationUnit(std::move(translationUnit_)), fatal(fatal_), indexed(indexed_) {}

  bool operator<(const StorageErrorData& other) const {
    if(message != other.message) {
      return message < other.message;
    } else if(translationUnit != other.translationUnit) {
      return translationUnit < other.translationUnit;
    } else if(fatal != other.fatal) {
      return fatal < other.fatal;
    } else {
      return indexed < other.indexed;
    }
  }

  std::wstring message = {};
  std::wstring translationUnit = {};
  bool fatal = false;
  bool indexed = false;
};

struct StorageError final : public StorageErrorData {
  StorageError() : StorageErrorData() {}

  StorageError(Id id_, const StorageErrorData& data) : StorageErrorData(data), id(id_) {}

  StorageError(Id id_, std::wstring message_, std::wstring translationUnit_, bool fatal_, bool indexed_)
      : StorageErrorData(std::move(message_), std::move(translationUnit_), fatal_, indexed_), id(id_) {}

  Id id = 0;
};
