#pragma once
// STL
#include <string>
// internal
#include "GlobalId.hpp"

struct StorageFile {
  StorageFile() = default;

  StorageFile(
      Id id_, std::wstring filePath_, std::wstring languageIdentifier_, std::string modificationTime_, bool indexed_, bool complete_)
      : id(id_)
      , filePath(std::move(filePath_))
      , languageIdentifier(std::move(languageIdentifier_))
      , modificationTime(std::move(modificationTime_))
      , indexed(indexed_)
      , complete(complete_) {}

  bool operator<(const StorageFile& other) const {
    return filePath < other.filePath;
  }

  Id id = 0;
  std::wstring filePath = {};
  std::wstring languageIdentifier = {};
  std::string modificationTime = {};
  bool indexed = true;
  bool complete = true;
};
