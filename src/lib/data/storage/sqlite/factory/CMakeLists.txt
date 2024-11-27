# ${CMAKE_CURRENT_LIST_DIR}/src/lib/data/storage/sqlite/factory/CMakeLists.txt
add_subdirectory(impls)

add_sourcetrail_library(
  NAME
  lib::data::storage::sqlite::factory::ISqliteFactory
  SOURCES
  ISqliteFactory.cpp
  PUBLIC_HEADERS
  ISqliteFactory.hpp)
