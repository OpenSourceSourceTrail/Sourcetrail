# ${CMAKE_SOURCE_DIR}/src/core/utility/file/utilityFile/CMakeLists.txt
add_sourcetrail_library(
  NAME
  core::utility::file::utilityFile
  SOURCES
  utilityFile.cpp
  PUBLIC_HEADERS
  utilityFile.h
  PRIVATE_DEPS
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::file::FileSystem
  Sourcetrail::core::utility::logging
  Sourcetrail::core::utility::utility)
