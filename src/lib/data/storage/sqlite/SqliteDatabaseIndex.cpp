#include "SqliteDatabaseIndex.h"

#include <utility>

#include "CppSQLite3.h"
#include "logging.h"

SqliteDatabaseIndex::SqliteDatabaseIndex(std::string indexName, std::string indexTarget) noexcept
    : mIndexName(std::move(indexName)), mIndexTarget(std::move(indexTarget)) {}

bool SqliteDatabaseIndex::createOnDatabase(CppSQLite3DB& database) noexcept {
  try {
    LOG_INFO(fmt::format("Creating database index \"{}\"", mIndexName));
    const auto query = fmt::format("CREATE INDEX IF NOT EXISTS {} ON {};", mIndexName, mIndexTarget);
    std::ignore = database.execDML(query.c_str());
  } catch(CppSQLite3Exception& error) {
    LOG_ERROR(fmt::format("{}: {}", error.errorCode(), error.errorMessage()));
    return false;
  }
  return true;
}

bool SqliteDatabaseIndex::removeFromDatabase(CppSQLite3DB& database) noexcept {
  try {
    LOG_INFO(fmt::format("Removing database index \"{}\"", mIndexName));
    const auto query = fmt::format("DROP INDEX IF EXISTS main.{};", mIndexName);
    std::ignore = database.execDML(query.c_str());
  } catch(CppSQLite3Exception& error) {
    LOG_ERROR(fmt::format("{}: {}", error.errorCode(), error.errorMessage()));
    return false;
  }
  return true;
}