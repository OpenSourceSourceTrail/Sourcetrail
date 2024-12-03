#ifndef STORAGE_BOOKMARK_CATEGORY_H
#define STORAGE_BOOKMARK_CATEGORY_H

#include <string>

#include "GlobalId.hpp"

struct StorageBookmarkCategoryData {
  StorageBookmarkCategoryData() = default;

  StorageBookmarkCategoryData(const std::wstring& name_) : name(name_) {}

  std::wstring name;
};

struct StorageBookmarkCategory : public StorageBookmarkCategoryData {
  StorageBookmarkCategory() = default;

  StorageBookmarkCategory(Id id_, const StorageBookmarkCategoryData& data) : StorageBookmarkCategoryData(data), id(id_) {}

  StorageBookmarkCategory(Id id_, const std::wstring& name_) : StorageBookmarkCategoryData(name_), id(id_) {}

  Id id = 0;
};

#endif    // STORAGE_BOOKMARK_CATEGORY_H
