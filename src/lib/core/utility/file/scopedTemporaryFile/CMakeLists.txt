# ${PROJECT_SOURCE_DIR}/src/core/utility/file/scopedTemporaryFile/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::file::ScopedTemporaryFile
  SOURCES ScopedTemporaryFile.cpp
  HEADERS ScopedTemporaryFile.hpp
  PRIVATE_DEPS fmt::fmt Sourcetrail::lib::core::utility::logging)
