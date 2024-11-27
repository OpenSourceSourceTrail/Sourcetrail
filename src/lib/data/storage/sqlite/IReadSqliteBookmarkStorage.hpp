#pragma once
#include <string>
#include <vector>

#include <nonstd/expected.hpp>

struct StorageBookmark;
struct StorageBookmarkedNode;
struct StorageBookmarkedEdge;
struct StorageBookmarkCategory;

namespace sqlite {

template <typename T>
using expected = nonstd::expected<T, std::string>;

/**
 * @brief Interface for ReadSqliteBookmarkStorage
 */
struct IReadSqliteBookmarkStorage {
  virtual ~IReadSqliteBookmarkStorage() noexcept;

  /**
   * @brief Retrieves all bookmarks from storage
   * @return Vector containing all stored bookmarks
   */
  [[nodiscard]] virtual expected<std::vector<StorageBookmark>> getAllBookmarks() const noexcept = 0;

  /**
   * @brief Retrieves all bookmarked nodes from storage
   * @return Vector containing all stored bookmarked nodes
   */
  [[nodiscard]] virtual expected<std::vector<StorageBookmarkedNode>> getAllBookmarkedNodes() const noexcept = 0;

  /**
   * @brief Retrieves all bookmarked edges from storage
   * @return Vector containing all stored bookmarked edges
   */
  [[nodiscard]] virtual expected<std::vector<StorageBookmarkedEdge>> getAllBookmarkedEdges() const noexcept = 0;

  /**
   * @brief Retrieves all bookmark categories from storage
   * @return Vector containing all stored bookmark categories
   */
  [[nodiscard]] virtual expected<std::vector<StorageBookmarkCategory>> getAllBookmarkCategories() const noexcept = 0;

  /**
   * @brief Retrieves a bookmark category by its name
   * @param name Name of the category to retrieve
   * @return The requested bookmark category
   */
  [[nodiscard]] virtual expected<StorageBookmarkCategory> getBookmarkCategoryByName(const std::wstring& name) const noexcept = 0;
};

}    // namespace sqlite
