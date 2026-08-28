#${CMAKE_SOURCE_DIR}/src/lib/core/CMakeLists.txt
add_subdirectory(utility)
add_subdirectory(http)

add_library(Sourcetrail_core INTERFACE)
add_library(Sourcetrail::lib::core ALIAS Sourcetrail_core)

target_include_directories(
  Sourcetrail_core
  INTERFACE ${CMAKE_SOURCE_DIR}/src/lib/lib/utility/ ${CMAKE_SOURCE_DIR}/src/lib/lib/app/paths/
            ${CMAKE_SOURCE_DIR}/src/lib/lib/utility/file/ ${CMAKE_SOURCE_DIR}/src/lib/lib/utility/text/)

target_link_libraries(Sourcetrail_core INTERFACE Boost::date_time Boost::system Boost::filesystem)

target_link_libraries(
  Sourcetrail_core
  INTERFACE Sourcetrail::lib::core::utility::math
            Sourcetrail::lib::core::utility::file::FilePath # TODO(Hussein): It must be moved to private
            Sourcetrail::lib::core::utility::file::FileInfo # TODO(Hussein): It must be moved to private
            Sourcetrail::lib::core::utility::TimeStamp # TODO(Hussein): It must be moved to private
            Sourcetrail::lib::core::utility::Version # TODO(Hussein): It must be moved to private
            Sourcetrail::lib::core::utility::UnorderedCache)

# $<$<PLATFORM_ID:Windows>:bcrypt>
target_compile_definitions(
  Sourcetrail_core INTERFACE $<$<PLATFORM_ID:Windows>:D_WINDOWS> $<$<PLATFORM_ID:Linux>:D_LINUX>
                             $<$<PLATFORM_ID:Darwin>:D_Darwin> $<$<PLATFORM_ID:Windows>:SPDLOG_WCHAR_TO_UTF8_SUPPORT>)

if(ENABLE_UNIT_TEST)
  add_subdirectory(tests)
endif()
