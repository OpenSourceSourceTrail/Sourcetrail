# ${CMAKE_SOURCE_DIR}/src/core/utility/fileSystem/impls/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::FileSystem
  SOURCES FileSystem.cpp
  HEADERS FileSystem.hpp
  PUBLIC_DEPS Sourcetrail::lib::core::utility::IFileSystem
  PRIVATE_DEPS fmt::fmt)
