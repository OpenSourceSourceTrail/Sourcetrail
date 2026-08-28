#pragma once

#include <string>
#include <vector>

#include "data/storage/sqlite/SqliteStorage.h"
#include "Migration.h"

class SqliteStorageMigration : public Migration<SqliteStorage> {
public:
  ~SqliteStorageMigration() override;

  bool executeStatementInStorage(SqliteStorage* storage, const std::string& statement) const;
};
