# ${CMAKE_SOURCE_DIR}/src/lib_gui/utility/utilityApp/CMakeLists.txt
add_sourcetrail_library(
  NAME
  libGui::utility::utilityApp
  SOURCES
  utilityApp.cpp
  PUBLIC_HEADERS
  utilityApp.h
  PUBLIC_DEPS
  Sourcetrail::core::utility::ApplicationArchitectureType
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::OsType
  PRIVATE_DEPS
  Qt6::Core
  Boost::filesystem
  Sourcetrail::core::utility::logging
  Sourcetrail::core::utility::ScopedFunctor
  Sourcetrail::core::utility::utilityString)
