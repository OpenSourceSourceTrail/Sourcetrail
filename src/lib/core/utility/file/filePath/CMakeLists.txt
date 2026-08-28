# ${CMAKE_SOURCE_DIR}/src/core/utility/file/filePath/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::file::FilePath
  SOURCES FilePath.cpp
  HEADERS FilePath.h
  PRIVATE_DEPS Boost::filesystem Sourcetrail::lib::core::utility::logging)
