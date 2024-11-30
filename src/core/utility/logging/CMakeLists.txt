# ${CMAKE_SOURCE_DIR}/src/core/utility/logging/CMakeLists.txt
add_sourcetrail_interface(
  NAME
  core::utility::logging
  DEPS
  spdlog::spdlog
  Sourcetrail::core::utility::utilityString)
target_compile_definitions(Sourcetrail_core_utility_logging INTERFACE SOURCE_PATH_PREFIX_LEN=${SOURCE_PATH_PREFIX_LEN})
