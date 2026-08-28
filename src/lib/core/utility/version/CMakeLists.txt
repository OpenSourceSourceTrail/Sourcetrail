# ${PROJECT_SOURCE_DIR}/src/core/utility/version/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::Version
  SOURCES Version.cpp
  HEADERS Version.h
  PRIVATE_DEPS fmt::fmt Sourcetrail::lib::core::utility::utilityString Sourcetrail::lib::core::utility::logging)
