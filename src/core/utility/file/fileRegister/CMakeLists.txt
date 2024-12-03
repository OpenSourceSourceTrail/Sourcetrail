# ${CMAKE_SOURCE_DIR}/src/core/utility/file/fileRegister/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::FileRegister
  SOURCES
  FileRegister.cpp
  PUBLIC_HEADERS
  FileRegister.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::UnorderedCache
  PRIVATE_DEPS
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::file::FilePathFilter)
