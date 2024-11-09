#pragma once
/**
 * @file SqliteDatabaseIndex.hpp
 * @brief Manages SQLite database index operations.
 */

#include <string>

class CppSQLite3DB;

/**
 * @brief Manages SQLite database index operations.
 *
 * This class handles the creation and removal of indexes in a SQLite database.
 * Each instance represents a single database index with a unique name and target.
 */
class SqliteDatabaseIndex final {
public:
  /**
   * @brief Constructs a new SQLite database index.
   * @param indexName The name of the index to be created/managed.
   * @param indexTarget The target definition of the index (e.g., table and columns).
   */
  SqliteDatabaseIndex(std::string indexName, std::string indexTarget) noexcept;

  /**
   * @brief Gets the name of the index.
   * @return The index name as a string.
   */
  [[nodiscard]] std::string getName() const noexcept {
    return mIndexName;
  }

  /**
   * @brief Creates the index in the specified database.
   * @param database Reference to the database connection.
   * @return true if the index was created successfully, false otherwise.
   */
  [[nodiscard]] bool createOnDatabase(CppSQLite3DB& database) noexcept;

  /**
   * @brief Removes the index from the specified database.
   * @param database Reference to the database connection.
   * @return true if the index was removed successfully, false otherwise.
   * @note The operation is safe to call even if the index doesn't exist (uses IF EXISTS).
   */
  [[nodiscard]] bool removeFromDatabase(CppSQLite3DB& database) noexcept;

private:
  std::string mIndexName;      ///< The name of the index
  std::string mIndexTarget;    ///< The target definition of the index
};