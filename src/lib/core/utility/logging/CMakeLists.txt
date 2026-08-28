# ${PROJECT_SOURCE_DIR}/src/lib/core/utility/logging/CMakeLists.txt
add_sourcetrail_interface(NAME lib::core::utility::logging DEPS spdlog::spdlog
                                                                Sourcetrail::lib::core::utility::utilityString)
target_compile_definitions(Sourcetrail_lib_core_utility_logging
                           INTERFACE SOURCE_PATH_PREFIX_LEN=${SOURCE_PATH_PREFIX_LEN})
