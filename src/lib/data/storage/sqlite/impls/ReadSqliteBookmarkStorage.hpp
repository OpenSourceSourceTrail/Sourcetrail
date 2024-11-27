#pragma once
#include <memory>
#include <vector>

#include "IReadSqliteBookmarkStorage.hpp"

class CppSQLite3DB;
struct SqliteFactory;

namespace sqlite {

class ReadSqliteBookmarkStorage final : public IReadSqliteBookmarkStorage {
public:
  ~ReadSqliteBookmarkStorage() noexcept override;

  ReadSqliteBookmarkStorage(const ReadSqliteBookmarkStorage&) = delete;
  ReadSqliteBookmarkStorage& operator=(const ReadSqliteBookmarkStorage&) = delete;

  ReadSqliteBookmarkStorage(ReadSqliteBookmarkStorage&&) = delete;
  ReadSqliteBookmarkStorage& operator=(ReadSqliteBookmarkStorage&&) = delete;

  [[nodiscard]] expected<std::vector<StorageBookmark>> getAllBookmarks() const noexcept override;

  [[nodiscard]] expected<std::vector<StorageBookmarkedNode>> getAllBookmarkedNodes() const noexcept override;

  [[nodiscard]] expected<std::vector<StorageBookmarkedEdge>> getAllBookmarkedEdges() const noexcept override;

  [[nodiscard]] expected<std::vector<StorageBookmarkCategory>> getAllBookmarkCategories() const noexcept override;

  [[nodiscard]] expected<StorageBookmarkCategory> getBookmarkCategoryByName(const std::wstring& name) const noexcept override;

protected:
  explicit ReadSqliteBookmarkStorage() noexcept;

private:
  std::unique_ptr<CppSQLite3DB> mDatabase;

  friend SqliteFactory;
};

}    // namespace sqlite