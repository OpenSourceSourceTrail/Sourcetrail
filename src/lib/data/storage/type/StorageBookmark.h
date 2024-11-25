#ifndef STORAGE_BOOKMARK_H
#define STORAGE_BOOKMARK_H

#include <string>

#include "GlobalId.hpp"

struct StorageBookmarkData {
  StorageBookmarkData() = default;

  StorageBookmarkData(const std::wstring& name_, const std::wstring& comment_, const std::string& timestamp_, const Id categoryId_)
      : name(name_), comment(comment_), timestamp(timestamp_), categoryId(categoryId_) {}

  std::wstring name;
  std::wstring comment;
  std::string timestamp;
  Id categoryId = 0;
};

struct StorageBookmark : public StorageBookmarkData {
  StorageBookmark() = default;

  StorageBookmark(Id id_, const StorageBookmarkData& data) : StorageBookmarkData(data), id(id_) {}

  StorageBookmark(Id id_, const std::wstring& name_, const std::wstring& comment_, const std::string& timestamp_, const Id categoryId_)
      : StorageBookmarkData(name_, comment_, timestamp_, categoryId_), id(id_) {}

  Id id = 0;
};

#endif    // STORAGE_BOOKMARK_H
