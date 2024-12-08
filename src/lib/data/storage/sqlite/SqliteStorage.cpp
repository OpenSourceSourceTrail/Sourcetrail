#include "SqliteStorage.h"

#include "FileSystem.h"
#include "logging.h"
#include "TimeStamp.h"
#include "utilityString.h"

SqliteStorage::SqliteStorage() {
  try {
    m_database.open(":memory:");
  } catch(CppSQLite3Exception& e) {
    LOG_ERROR_W(L"Failed to load database file \":memory:\" with message: " + utility::decodeFromUtf8(e.errorMessage()));
    throw;
  }

  executeStatement("PRAGMA foreign_keys=ON;");
}

SqliteStorage::SqliteStorage(const FilePath& dbFilePath) : m_dbFilePath(dbFilePath.getCanonical()) {
  if(!m_dbFilePath.getParentDirectory().empty() && !m_dbFilePath.getParentDirectory().exists()) {
    filesystem::createDirectory(m_dbFilePath.getParentDirectory());
  }

  try {
    m_database.open(utility::encodeToUtf8(m_dbFilePath.wstr()).c_str());
  } catch(CppSQLite3Exception& e) {
    LOG_ERROR_W(L"Failed to load database file \"" + m_dbFilePath.wstr() + L"\" with message: " +
                utility::decodeFromUtf8(e.errorMessage()));
    throw;
  }

  executeStatement("PRAGMA foreign_keys=ON;");
}

SqliteStorage::~SqliteStorage() {
  try {
    m_database.close();
  } catch(CppSQLite3Exception& e) {
    LOG_ERROR(e.errorMessage());
  }
}

void SqliteStorage::setup() {
  executeStatement("PRAGMA foreign_keys=ON;");
  setupMetaTable();

  if(isEmpty() || !isIncompatible()) {
    setupTables();

    if(!m_precompiledStatementsInitialized) {
      setupPrecompiledStatements();
      m_precompiledStatementsInitialized = true;
    }
  }
}

void SqliteStorage::clear() {
  executeStatement("PRAGMA foreign_keys=OFF;");
  clearMetaTable();
  clearTables();

  setup();
}

size_t SqliteStorage::getVersion() const {
  if(const std::string storageVersionStr = getMetaValue("storage_version"); !storageVersionStr.empty()) {
    return static_cast<size_t>(std::stoi(storageVersionStr));
  }

  return 0;
}

void SqliteStorage::setVersion(size_t version) {
  insertOrUpdateMetaValue("storage_version", std::to_string(version));
}

void SqliteStorage::beginTransaction() {
  executeStatement("BEGIN TRANSACTION;");
}

void SqliteStorage::commitTransaction() {
  executeStatement("COMMIT TRANSACTION;");
}

void SqliteStorage::rollbackTransaction() {
  executeStatement("ROLLBACK TRANSACTION;");
}

void SqliteStorage::optimizeMemory() const {
  executeStatement("VACUUM;");
}

bool SqliteStorage::isEmpty() const {
  return getVersion() <= 0;
}

bool SqliteStorage::isIncompatible() const {
  return isEmpty() || getVersion() != getStaticVersion();
}

void SqliteStorage::setTime() {
  insertOrUpdateMetaValue("timestamp", TimeStamp::now().toString());
}

TimeStamp SqliteStorage::getTime() const {
  return {getMetaValue("timestamp")};
}

void SqliteStorage::setupMetaTable() {
  try {
    std::ignore = m_database.execDML(
        "CREATE TABLE IF NOT EXISTS meta("
        "id INTEGER, "
        "key TEXT, "
        "value TEXT, "
        "PRIMARY KEY(id)"
        ");");
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));

    throw(std::exception());
  }
}

void SqliteStorage::clearMetaTable() {
  try {
    std::ignore = m_database.execDML("DROP TABLE IF EXISTS main.meta;");
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
  }
}

bool SqliteStorage::executeStatement(const std::string& statement) const {
  try {
    std::ignore = m_database.execDML(statement.c_str());
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
    return false;
  }
  return true;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool SqliteStorage::executeStatement(CppSQLite3Statement& statement) const {
  try {
    std::ignore = statement.execDML();
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
    return false;
  }

  statement.reset();
  return true;
}

int SqliteStorage::executeStatementScalar(const std::string& statement, const int nullValue) const {
  int ret = 0;
  try {
    ret = m_database.execScalar(statement.c_str(), nullValue);
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
  }
  return ret;
}

int SqliteStorage::executeStatementScalar(CppSQLite3Statement& statement, const int nullValue) const {
  int ret = 0;
  try {
    CppSQLite3Query query = executeQuery(statement);
    if(query.eof() || query.numFields() < 1) {
      // NOLINTNEXTLINE(hicpp-exception-baseclass)
      throw CppSQLite3Exception(CPPSQLITE_ERROR, "Invalid scalar query", false);
    }

    ret = query.getIntField(0, nullValue);
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
  }

  return ret;
}

CppSQLite3Query SqliteStorage::executeQuery(const std::string& statement) const {
  try {
    return m_database.execQuery(statement.c_str());
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
  }
  return {};
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
CppSQLite3Query SqliteStorage::executeQuery(CppSQLite3Statement& statement) const {
  try {
    return statement.execQuery();
  } catch(CppSQLite3Exception& exception) {
    LOG_ERROR(fmt::format("{}: {}", exception.errorCode(), exception.errorMessage()));
  }
  return {};
}

bool SqliteStorage::hasTable(const std::string& tableName) const {
  if(CppSQLite3Query query = executeQuery(
         fmt::format("SELECT name FROM sqlite_master WHERE type='table' AND name='{}';", tableName));
     !query.eof()) {
    return query.getStringField(0, "") == tableName;
  }

  return false;
}

std::string SqliteStorage::getMetaValue(const std::string& key) const {
  if(hasTable("meta")) {
    if(CppSQLite3Query query = executeQuery(fmt::format("SELECT value FROM meta WHERE key = '{}';", key)); !query.eof()) {
      return query.getStringField(0, "");
    }
  }

  return "";
}

void SqliteStorage::insertOrUpdateMetaValue(const std::string& key, const std::string& value) {
  CppSQLite3Statement stmt = m_database.compileStatement(std::string("INSERT OR REPLACE INTO meta(id, key, value) VALUES("
                                                                     "(SELECT id FROM meta WHERE key = ?), ?, ?"
                                                                     ");")
                                                             .c_str());

  stmt.bind(1, key.c_str());
  stmt.bind(2, key.c_str());
  stmt.bind(3, value.c_str());
  executeStatement(stmt);
}
