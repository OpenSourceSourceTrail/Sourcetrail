#pragma once

#include <string>

class CppSQLite3DB;

class SqliteDatabaseIndex final {
public:
  SqliteDatabaseIndex(std::string indexName, std::string indexTarget);

  [[nodiscard]] std::string getName() const {
    return mIndexName;
  }

  bool createOnDatabase(CppSQLite3DB& database);
  bool removeFromDatabase(CppSQLite3DB& database);

private:
  std::string mIndexName;
  std::string mIndexTarget;
};