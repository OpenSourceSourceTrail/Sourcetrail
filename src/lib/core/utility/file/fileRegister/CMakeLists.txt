# ${PROJECT_SOURCE_DIR}/src/core/utility/file/fileRegister/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::file::FileRegister
  SOURCES FileRegister.cpp
  HEADERS FileRegister.h
  PUBLIC_DEPS Sourcetrail::lib::core::utility::UnorderedCache
  PRIVATE_DEPS Sourcetrail::lib::core::utility::file::FilePath Sourcetrail::lib::core::utility::file::FilePathFilter)
