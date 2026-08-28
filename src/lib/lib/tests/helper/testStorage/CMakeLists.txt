# src\test\helper\testStorage\CMakeLists.txt
add_sourcetrail_library(
  NAME helper::TestStorage
  SOURCES TestStorage.cpp
  HEADERS TestStorage.h
  PUBLIC_DEPS
  PRIVATE_DEPS Sourcetrail::lib::lib Sourcetrail::lib::core::utility::file::FilePath
               Sourcetrail::lib::core::utility::utilityString)
