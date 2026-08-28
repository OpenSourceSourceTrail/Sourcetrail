# ${CMAKE_SOURCE_DIR}/src/core/utility/utilityUuid/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::utilityUuid
  SOURCES utilityUuid.cpp
  HEADERS utilityUuid.h
  PRIVATE_DEPS Boost::system)
