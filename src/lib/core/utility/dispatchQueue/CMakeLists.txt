# ${PROJECT_SOURCE_DIR}/src/core/utility/dispatchQueue/CMakeLists.txt
add_sourcetrail_library(
  NAME lib::core::utility::DispatchQueue
  SOURCES DispatchQueue.cpp DispatchRegistry.cpp
  HEADERS DispatchQueue.h DispatchRegistry.h
  PUBLIC_DEPS Sourcetrail::lib::core::utility::GlobalId)
