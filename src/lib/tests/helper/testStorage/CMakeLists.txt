# src\test\helper\testStorage\CMakeLists.txt
add_sourcetrail_library(
  NAME
  helper::TestStorage
  SOURCES
  TestStorage.cpp
  PUBLIC_HEADERS
  TestStorage.h
  PUBLIC_DEPS
  PRIVATE_DEPS
  Sourcetrail::lib
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::utilityString)
