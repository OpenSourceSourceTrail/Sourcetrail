# ${CMAKE_CURRENT_LIST_DIR}/src/lib/data/storage/sqlite/CMakeLists.txt
add_subdirectory(factory)
add_subdirectory(impls)

add_sourcetrail_library(
  NAME
  lib::data::storage::sqlite::IReadSqliteBookmarkStorage
  SOURCES
  IReadSqliteBookmarkStorage.cpp
  PUBLIC_HEADERS
  IReadSqliteBookmarkStorage.hpp
  PUBLIC_DEPS
  nonstd::expected-lite)
