# ${PROJECT_SOURCE_DIR}/src/core/utility/TextCodec/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::TextCodec
  SOURCES TextCodec.cpp
  HEADERS TextCodec.h
  PRIVATE_DEPS Boost::locale)
