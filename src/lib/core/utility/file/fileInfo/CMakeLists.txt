# ${CMAKE_SOURCE_DIR}/src/core/utility/file/fileInfo/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::file::FileInfo
  SOURCES FileInfo.cpp
  PUBLIC_HEADERS FileInfo.h
  PUBLIC_DEPS Sourcetrail::lib::core::utility::file::FilePath Sourcetrail::lib::core::utility::TimeStamp)
