#pragma once
#include <filesystem>


namespace sqlite {

struct IReadSqliteBookmarkStorage;

struct ISqliteFactory {
  virtual ~ISqliteFactory() noexcept;
  virtual std::unique_ptr<IReadSqliteBookmarkStorage> createReadSqliteBookmarkStorage(const std::filesystem::path& path) noexcept = 0;
};

}    // namespace sqlite