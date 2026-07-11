# helper\testFileRegister\CMakeLists.txt
add_sourcetrail_library(
  NAME
  helper::TestFileRegister
  SOURCES
  TestFileRegister.cpp
  PUBLIC_HEADERS
  TestFileRegister.h
  PUBLIC_DEPS
  Sourcetrail::lib::core::utility::file::FileRegister
  PRIVATE_DEPS
  Sourcetrail::lib::core::utility::file::FilePathFilter)
