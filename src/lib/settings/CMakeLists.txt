# ${CMAKE_SOURCE_DIR}/src/lib/settings/CMakeLists.txt
add_library(Sourcetrail_lib_settings)
add_library(Sourcetrail::lib::settings ALIAS Sourcetrail_lib_settings)

target_sources(Sourcetrail_lib_settings PRIVATE Settings.cpp)

target_include_directories(Sourcetrail_lib_settings PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(
  Sourcetrail_lib_settings
  PUBLIC
  Sourcetrail::storage::type
  Sourcetrail::core)
# ========================================================