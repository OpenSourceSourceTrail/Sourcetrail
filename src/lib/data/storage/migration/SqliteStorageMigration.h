#pragma once

#include <string>
#include <vector>

#include "Migration.h"
#include "SqliteStorage.h"

class SqliteStorageMigration : public Migration<SqliteStorage> {
public:
  ~SqliteStorageMigration() override;

  bool executeStatementInStorage(SqliteStorage* storage, const std::string& statement) const;
};
