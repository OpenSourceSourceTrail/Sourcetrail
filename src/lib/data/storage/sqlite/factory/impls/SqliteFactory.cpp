#include "SqliteFactory.hpp"

#include <fmt/core.h>

#include <CppSQLite3.h>

#include "logging.h"
#include "ReadSqliteBookmarkStorage.hpp"

namespace sqlite {

SqliteFactory::~SqliteFactory() noexcept = default;

std::unique_ptr<IReadSqliteBookmarkStorage> SqliteFactory::createReadSqliteBookmarkStorage(const std::filesystem::path& path) noexcept {
  auto database = std::make_unique<CppSQLite3DB>();
  if(std::error_code errorCode; !std::filesystem::exists(path, errorCode)) {
    LOG_ERROR(fmt::format("The file doesn't exists \"{}\"", path.string()));
    return nullptr;
  }

  try {
    database->openReadOnly(path.string().c_str());
    std::unique_ptr<ReadSqliteBookmarkStorage> storage{new ReadSqliteBookmarkStorage()};
    storage->mDatabase = std::move(database);
    return storage;
  } catch(CppSQLite3Exception& exp) {
    LOG_ERROR(fmt::format("Failed to load database file \"{}\" with message: {}", path.string(), exp.what()));
  } catch(...) {
    LOG_ERROR(fmt::format("Failed to load database file \"{}\"", path.string()));
  }
  return nullptr;
}

}    // namespace sqlite