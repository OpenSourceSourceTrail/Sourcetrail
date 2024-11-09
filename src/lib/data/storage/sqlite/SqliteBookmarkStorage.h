#pragma once
/**
 * @file SqliteBookmarkStorage.h
 * @brief SQLite-based storage implementation for bookmarks and bookmark-related data
 */

#include "GlobalId.hpp"
#include "SqliteDatabaseIndex.h"
#include "SqliteStorage.h"
#include "StorageBookmark.h"
#include "StorageBookmarkCategory.h"
#include "StorageBookmarkedEdge.h"
#include "StorageBookmarkedNode.h"

/**
 * @class SqliteBookmarkStorage
 * @brief Manages persistent storage of bookmarks, bookmark categories, and bookmarked elements using SQLite
 *
 * This class provides functionality to store and retrieve bookmarks and related data
 * in a SQLite database. It handles both file-based and in-memory database storage.
 */
class SqliteBookmarkStorage : public SqliteStorage {
public:
  /**
   * @brief Construct a new SqliteBookmarkStorage object
   *
   * This constructor is used for in-memory SQLite databases.
   * @note For testing purposes only.
   */
  SqliteBookmarkStorage();

  /**
   * @brief Constructs a SqliteBookmarkStorage with a specific database file
   * @param dbFilePath Path to the SQLite database file
   */
  explicit SqliteBookmarkStorage(const FilePath& dbFilePath);

  /**
   * @brief Gets the static version number of the storage schema
   * @return The version number of the storage schema
   */
  size_t getStaticVersion() const override;

  /**
   * @brief Performs database migration if the schema version has changed
   */
  void migrateIfNecessary();

  /**
   * @brief Adds a new bookmark category to the storage
   * @param data The bookmark category data to store
   * @return The stored bookmark category with its assigned ID
   */
  StorageBookmarkCategory addBookmarkCategory(const StorageBookmarkCategoryData& data);

  /**
   * @brief Adds a new bookmark to the storage
   * @param data The bookmark data to store
   * @return The stored bookmark with its assigned ID
   */
  StorageBookmark addBookmark(const StorageBookmarkData& data);

  /**
   * @brief Adds a bookmarked node to the storage
   * @param data The bookmarked node data to store
   * @return The stored bookmarked node with its assigned ID
   */
  StorageBookmarkedNode addBookmarkedNode(const StorageBookmarkedNodeData& data);

  /**
   * @brief Adds a bookmarked edge to the storage
   * @param data The bookmarked edge data to store
   * @return The stored bookmarked edge with its assigned ID
   */
  StorageBookmarkedEdge addBookmarkedEdge(const StorageBookmarkedEdgeData data);

  /**
   * @brief Removes a bookmark category from storage
   * @param id ID of the bookmark category to remove
   */
  void removeBookmarkCategory(Id id);

  /**
   * @brief Removes a bookmark from storage
   * @param id ID of the bookmark to remove
   */
  void removeBookmark(Id id);

  /**
   * @brief Retrieves all bookmarks from storage
   * @return Vector containing all stored bookmarks
   */
  std::vector<StorageBookmark> getAllBookmarks() const;

  /**
   * @brief Retrieves all bookmarked nodes from storage
   * @return Vector containing all stored bookmarked nodes
   */
  std::vector<StorageBookmarkedNode> getAllBookmarkedNodes() const;

  /**
   * @brief Retrieves all bookmarked edges from storage
   * @return Vector containing all stored bookmarked edges
   */
  std::vector<StorageBookmarkedEdge> getAllBookmarkedEdges() const;

  /**
   * @brief Updates an existing bookmark's properties
   * @param bookmarkId ID of the bookmark to update
   * @param name New name for the bookmark
   * @param comment New comment for the bookmark
   * @param categoryId New category ID for the bookmark
   */
  void updateBookmark(const Id bookmarkId, const std::wstring& name, const std::wstring& comment, const Id categoryId);

  /**
   * @brief Retrieves all bookmark categories from storage
   * @return Vector containing all stored bookmark categories
   */
  std::vector<StorageBookmarkCategory> getAllBookmarkCategories() const;

  /**
   * @brief Retrieves a bookmark category by its name
   * @param name Name of the category to retrieve
   * @return The requested bookmark category
   */
  StorageBookmarkCategory getBookmarkCategoryByName(const std::wstring& name) const;

private:
  /** @brief Current storage schema version */
  static const size_t s_storageVersion;

  /**
   * @brief Gets the database indices configuration
   * @return Vector of pairs containing index version and index definition
   */
  virtual std::vector<std::pair<int, SqliteDatabaseIndex>> getIndices() const;
  /**
   * @brief Clears all tables in the database
   */
  void clearTables() override;
  /**
   * @brief Sets up the database tables
   */
  void setupTables() override;
  /**
   * @brief Prepares SQL statements for common operations
   */
  void setupPrecompiledStatements() override;

  /**
   * @brief Template method to retrieve all items of a specific type
   * @tparam ResultType The type of items to retrieve
   * @param query The SQL query to execute
   * @return Vector of retrieved items
   */
  template <typename ResultType>
  std::vector<ResultType> doGetAll(const std::string& query) const;

  /**
   * @brief Template method to retrieve the first item of a specific type
   * @tparam ResultType The type of item to retrieve
   * @param query The SQL query to execute
   * @return The first retrieved item or an empty item if none found
   */
  template <typename ResultType>
  ResultType doGetFirst(const std::string& query) const {
    std::vector<ResultType> results = doGetAll<ResultType>(query + " LIMIT 1");
    if(!results.empty()) {
      return results.front();
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