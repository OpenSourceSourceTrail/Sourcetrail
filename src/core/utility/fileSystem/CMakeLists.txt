# ${CMAKE_SOURCE_DIR}/src/core/utility/fileSystem/CMakeLists.txt
add_subdirectory(impls)
if(ENABLE_UNIT_TEST OR ENABLE_GUI_TEST OR ENABLE_INTEGRATION_TEST)
  add_subdirectory(mocks)
endif()

add_sourcetrail_library(
  NAME
  core::utility::IFileSystem
  SOURCES
  IFileSystem.cpp
  PUBLIC_HEADERS
  IFileSystem.hpp
  PUBLIC_DEPS
  nonstd::expected-lite)
