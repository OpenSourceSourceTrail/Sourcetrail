#include "SqliteStorageMigration.h"

SqliteStorageMigration::~SqliteStorageMigration() = default;

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool SqliteStorageMigration::executeStatementInStorage(SqliteStorage* storage, const std::string& statement) const {
  return storage->executeStatement(statement);
}
