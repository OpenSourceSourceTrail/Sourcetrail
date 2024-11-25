#pragma once

#include "GlobalId.hpp"
#include "SqliteStorage.h"
#include "StorageBookmark.h"
#include "StorageBookmarkCategory.h"
#include "StorageBookmarkedEdge.h"
#include "StorageBookmarkedNode.h"

class SqliteBookmarkStorage : public SqliteStorage {
public:
  explicit SqliteBookmarkStorage(const FilePath& dbFilePath);

  size_t getStaticVersion() const override;

  void migrateIfNecessary();

  StorageBookmarkCategory addBookmarkCategory(const StorageBookmarkCategoryData& data);
  StorageBookmark addBookmark(const StorageBookmarkData& data);
  StorageBookmarkedNode addBookmarkedNode(const StorageBookmarkedNodeData& data);
  StorageBookmarkedEdge addBookmarkedEdge(const StorageBookmarkedEdgeData data);

  void removeBookmarkCategory(Id id);
  void removeBookmark(Id id);

  std::vector<StorageBookmark> getAllBookmarks() const;
  std::vector<StorageBookmarkedNode> getAllBookmarkedNodes() const;
  std::vector<StorageBookmarkedEdge> getAllBookmarkedEdges() const;

  void updateBookmark(const Id bookmarkId, const std::wstring& name, const std::wstring& comment, const Id categoryId);

  std::vector<StorageBookmarkCategory> getAllBookmarkCategories() const;
  StorageBookmarkCategory getBookmarkCategoryByName(const std::wstring& name) const;

private:
  static const size_t s_storageVersion;

  virtual std::vector<std::pair<int, SqliteDatabaseIndex>> getIndices() const;
  void clearTables() override;
  void setupTables() override;
  void setupPrecompiledStatements() override;

  // void updateBookmarkMetaData(const BookmarkMetaData& metaData);

  template <typename ResultType>
  std::vector<ResultType> doGetAll(const std::string& query) const;

  template <typename ResultType>
  ResultType doGetFirst(const std::string& query) const {
    std::vector<ResultType> results = doGetAll<ResultType>(query + " LIMIT 1");
    if(!results.empty()) {
      return results[0];
    }
    return ResultType();
  }
};

template <>
std::vector<StorageBookmarkCategory> SqliteBookmarkStorage::doGetAll<StorageBookmarkCategory>(const std::string& query) const;
template <>
std::vector<StorageBookmark> SqliteBookmarkStorage::doGetAll<StorageBookmark>(const std::string& query) const;
template <>
std::vector<StorageBookmarkedNode> SqliteBookmarkStorage::doGetAll<StorageBookmarkedNode>(const std::string& query) const;
template <>
std::vector<StorageBookmarkedEdge> SqliteBookmarkStorage::doGetAll<StorageBookmarkedEdge>(const std::string& query) const;