# ${CMAKE_SOURCE_DIR}/src/core/utility/file/utilityFile/CMakeLists.txt
add_sourcetrail_library(
  NAME
  lib::core::utility::file::utilityFile
  SOURCES
  utilityFile.cpp
  PUBLIC_HEADERS
  utilityFile.h
  PRIVATE_DEPS
  Sourcetrail::lib::core::utility::file::FilePath
  Sourcetrail::lib::core::utility::file::FileSystem
  Sourcetrail::lib::core::utility::logging
  Sourcetrail::lib::core::utility::utility)
