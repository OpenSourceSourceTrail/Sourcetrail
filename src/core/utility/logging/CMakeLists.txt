# ${CMAKE_SOURCE_DIR}/src/core/utility/logging/CMakeLists.txt
add_library(Sourcetrail_core_utility_logging INTERFACE)
add_library(Sourcetrail::core::utility::logging ALIAS Sourcetrail_core_utility_logging)

target_link_libraries(Sourcetrail_core_utility_logging INTERFACE spdlog::spdlog
                                                                 Sourcetrail::core::utility::utilityString)
target_include_directories(Sourcetrail_core_utility_logging INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(Sourcetrail_core_utility_logging INTERFACE SOURCE_PATH_PREFIX_LEN=${SOURCE_PATH_PREFIX_LEN})
