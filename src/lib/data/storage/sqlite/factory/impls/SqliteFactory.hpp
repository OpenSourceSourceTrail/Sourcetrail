#pragma once
#include <filesystem>

#include "IReadSqliteBookmarkStorage.hpp"
#include "ISqliteFactory.hpp"

namespace sqlite {

struct SqliteFactory final : ISqliteFactory {
  ~SqliteFactory() noexcept override;
  std::unique_ptr<IReadSqliteBookmarkStorage> createReadSqliteBookmarkStorage(const std::filesystem::path& path) noexcept override;
};

}    // namespace sqlite