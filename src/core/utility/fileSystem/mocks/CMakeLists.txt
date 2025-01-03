# ${CMAKE_SOURCE_DIR}/src/core/utility/fileSystem/mocks/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::MockedFileSystem
  SOURCES
  MockedFileSystem.cpp
  PUBLIC_HEADERS
  MockedFileSystem.hpp
  PUBLIC_DEPS
  Sourcetrail::core::utility::IFileSystem
  GTest::gmock)
