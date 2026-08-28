# ${PROJECT_SOURCE_DIR}/src/core/utility/file/fileSystem/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::file::FileSystem
  SOURCES FileSystem.cpp
  HEADERS FileSystem.h
  PUBLIC_DEPS Sourcetrail::lib::core::utility::file::FileInfo Sourcetrail::lib::core::utility::TimeStamp
  PRIVATE_DEPS Boost::system Sourcetrail::lib::core::utility::utilityString)
