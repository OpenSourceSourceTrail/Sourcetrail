# ${CMAKE_CURRENT_LIST_DIR}/src/lib/data/storage/sqlite/factory/impls/CMakeLists.txt
add_sourcetrail_library(
  NAME
  lib::data::storage::sqlite::factory::impls::SqliteFactory
  SOURCES
  SqliteFactory.cpp
  PUBLIC_HEADERS
  SqliteFactory.hpp
  PUBLIC_DEPS
  Sourcetrail::lib::data::storage::sqlite::factory::ISqliteFactory
  Sourcetrail::lib::data::storage::sqlite::IReadSqliteBookmarkStorage
  PRIVATE_DEPS
  Sourcetrail::lib::data::storage::sqlite::impls::ReadSqliteBookmarkStorage
  Sourcetrail::core
  CppSQLite::CppSQLite3)
