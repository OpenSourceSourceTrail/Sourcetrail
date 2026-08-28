# ${PROJECT_SOURCE_DIR}/src/core/utility/file/filePathFilter/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::file::FilePathFilter
  SOURCES FilePathFilter.cpp
  HEADERS FilePathFilter.h
  PUBLIC_DEPS Sourcetrail::lib::core::utility::file::FilePath)
