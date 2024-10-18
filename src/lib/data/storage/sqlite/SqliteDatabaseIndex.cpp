#include "SqliteDatabaseIndex.h"

#include <utility>

#include "CppSQLite3.h"
#include "logging.h"

SqliteDatabaseIndex::SqliteDatabaseIndex(std::string indexName, std::string indexTarget)
    : mIndexName(std::move(indexName)), mIndexTarget(std::move(indexTarget)) {}

bool SqliteDatabaseIndex::createOnDatabase(CppSQLite3DB& database) {
  try {
    LOG_INFO(fmt::format("Creating database index \"{}\"", mIndexName));
    database.execDML(("CREATE INDEX IF NOT EXISTS " + mIndexName + " ON " + mIndexTarget + ";").c_str());
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
    return false;
  }
  return true;
}

bool SqliteDatabaseIndex::removeFromDatabase(CppSQLite3DB& database) {
  try {
    LOG_INFO(fmt::format("Removing database index \"{}\"", mIndexName));
    database.execDML(("DROP INDEX IF EXISTS main." + mIndexName + ";").c_str());
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
    return false;
  }
  return true;
}
