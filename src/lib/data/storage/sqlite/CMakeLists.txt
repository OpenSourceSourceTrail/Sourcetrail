# Sourcetrail_lib_data_storage_sqlite
add_sourcetrail_library(
  NAME
  lib::data::storage::sqlite::IReadSqliteBookmarkStorage
  SOURCES
  IReadSqliteBookmarkStorage.cpp
  PUBLIC_HEADERS
  IReadSqliteBookmarkStorage.hpp
  PUBLIC_DEPS
  nonstd::expected-lite
  WARNING_AS_ERROR
  ${SOURCETRAIL_WARNING_AS_ERROR})
