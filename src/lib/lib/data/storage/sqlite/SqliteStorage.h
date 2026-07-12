#pragma once
/**
 * @file SqliteStorage.h
 * @brief This file contains the SqliteStorage class, which is a base class for SQLite-based storage.
 */

#include "CppSQLite3.h"    // TODO(Hussein): Should be removed
#include "FilePath.h"

class SqliteStorageMigration;
class TimeStamp;

/**
 * @brief The SqliteStorage class is a base class for SQLite-based storage.
 */
class SqliteStorage {
public:
  /**
   * @brief Construct a new SqliteStorage object
   *
   * This constructor is used for in-memory SQLite databases.
   * @note For testing purposes only.
   */
  SqliteStorage();

  /**
   * @brief Construct a new SqliteStorage object
   *
   * This constructor is used for SQLite databases that are stored on the file system.
   * @throw CppSQLite3Exception
   */
  explicit SqliteStorage(const FilePath& dbFilePath);

  /**
   * @brief Destroy the SqliteStorage object
   */
  virtual ~SqliteStorage();

  /**
   * @brief Setup the storage
   * - Create Meta table
   * - Create Tables in case of empty db.
   * @throws CppSQLite3Exception, std::exception
   */
  void setup();

  /**
   * @brief Clear the storage
   */
  void clear();

  /**
   * @brief Get the version of the storage
   * @throw std::out_of_range
   * @return The version of the storage
   */
  size_t getVersion() const;

  /**
   * @brief Set the version of the storage
   * @param version The version to set
   */
  void setVersion(size_t version);

  /**
   * @brief Begin a transaction
   */
  void beginTransaction();

  /**
   * @brief Commit a transaction
   */
  void commitTransaction();

  /**
   * @brief Rollback a transaction
   */
  void rollbackTransaction();

  /**
   * @brief Optimize the memory
   */
  void optimizeMemory() const;

  /**
   * @brief Get the file path of the database
   * @return The file path of the database
   */
  [[nodiscard]] FilePath getDbFilePath() const noexcept {
    return m_dbFilePath;
  }

  /**
   * @brief Check if the database is empty
   * @throw std::out_of_range
   * @return True if the database is empty, false otherwise
   */
  bool isEmpty() const;

  /**
   * @brief Check if the database is incompatible
   * @return True if the database is incompatible, false otherwise
   */
  bool isIncompatible() const;

  /**
   * @brief Set the time
   */
  void setTime();

  /**
   * @brief Get the time
   * @return The time
   */
  TimeStamp getTime() const;

protected:
  /**
   * @brief Setup the meta table
   */
  void setupMetaTable();

  /**
   * @brief Clear the meta table
   */
  void clearMetaTable();

  /**
   * @brief Execute a statement
   * @param statement The statement to execute
   * @return True if the statement was executed successfully, false otherwise
   */
  bool executeStatement(const std::string& statement) const;

  /**
   * @brief Execute a statement
   * @param statement The statement to execute
   * @return True if the statement was executed successfully, false otherwise
   */
  bool executeStatement(CppSQLite3Statement& statement) const;

  /**
   * @brief Execute a statement and return a scalar value
   * @param statement The statement to execute
   * @param nullValue The value to return if the statement returns no rows
   * @return The scalar value
   */
  int executeStatementScalar(const std::string& statement, const int nullValue) const;

  /**
   * @brief Execute a statement and return a scalar value
   * @param statement The statement to execute
   * @param nullValue The value to return if the statement returns no rows
   * @return The scalar value
   */
  int executeStatementScalar(CppSQLite3Statement& statement, const int nullValue) const;

  /**
   * @brief Execute a query
   * @param statement The statement to execute
   * @return The query result
   */
  CppSQLite3Query executeQuery(const std::string& statement) const;

  /**
   * @brief Execute a query
   * @param statement The statement to execute
   * @return The query result
   */
  CppSQLite3Query executeQuery(CppSQLite3Statement& statement) const;

  /**
   * @brief Check if the table exists
   * @param tableName The name of the table to check
   * @return True if the table exists, false otherwise
   */
  bool hasTable(const std::string& tableName) const;

  /**
   * @brief Get the meta value
   * @param key The key to get the value for
   * @return The meta value
   */
  std::string getMetaValue(const std::string& key) const;

  /**
   * @brief Insert or update a meta value
   * @param key The key to insert or update
   * @param value The value to insert or update
   */
  void insertOrUpdateMetaValue(const std::string& key, const std::string& value);

  mutable CppSQLite3DB m_database;
  FilePath m_dbFilePath;

private:
  virtual size_t getStaticVersion() const = 0;
  virtual void clearTables() = 0;
  virtual void setupTables() = 0;
  virtual void setupPrecompiledStatements() = 0;

  bool m_precompiledStatementsInitialized = false;

  friend SqliteStorageMigration;
};
