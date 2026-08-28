# ${PROJECT_SOURCE_DIR}/src/core/utility/taskExecutor/CMakeLists.txt
find_package(Taskflow REQUIRED)

add_sourcetrail_library(
  NAME lib::core::utility::TaskExecutor
  SOURCES TaskExecutor.cpp
  HEADERS TaskExecutor.h
  PUBLIC_DEPS Taskflow::Taskflow)
