# ${PROJECT_SOURCE_DIR}/src/core/utility/fileSystem/mocks/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::MockedFileSystem
  SOURCES MockedFileSystem.cpp
  HEADERS MockedFileSystem.hpp
  PUBLIC_DEPS Sourcetrail::lib::core::utility::IFileSystem GTest::gmock)
