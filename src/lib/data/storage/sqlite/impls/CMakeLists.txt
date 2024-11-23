# ${CMAKE_CURRENT_LIST_DIR}/src/lib/data/storage/sqlite/impls/CMakeLists.txt
add_sourcetrail_library(
  NAME
  lib::data::storage::sqlite::impls::ReadSqliteBookmarkStorage
  SOURCES
  ReadSqliteBookmarkStorage.cpp
  PUBLIC_HEADERS
  ReadSqliteBookmarkStorage.hpp
  PUBLIC_DEPS
  Sourcetrail::lib::data::storage::sqlite::IReadSqliteBookmarkStorage
  PRIVATE_DEPS
  Sourcetrail::core
  Sourcetrail::lib
  CppSQLite::CppSQLite3)
