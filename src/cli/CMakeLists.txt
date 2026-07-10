#${CMAKE_SOURCE_DIR}/src/cli/CMakeLists.txt
add_executable(Sourcetrail_cli main.cpp ConsoleApplication.cpp ConsoleApplication.h)

set_target_properties(Sourcetrail_cli PROPERTIES OUTPUT_NAME sourcetrail_cli RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app/")

target_link_libraries(
  Sourcetrail_cli
  PRIVATE Sourcetrail::core
          Sourcetrail::lib
          Sourcetrail::proto::convert
          Sourcetrail::lib::utility::PathDetection
          Sourcetrail::messaging
          $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
          Sourcetrail::core::utility::logging
          Sourcetrail::core::utility::ScopedFunctor
          Sourcetrail::core::utility::utilityApp
          Sourcetrail::core::utility::commandline::CommandLineParser)

target_include_directories(
  Sourcetrail_cli
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
          "${CMAKE_BINARY_DIR}/src/lib"
          "${CMAKE_BINARY_DIR}/src/lib_gui")

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail_cli PRIVATE ${CORE_FOUNDATION})
endif()

add_dependencies(Sourcetrail_cli Sourcetrail_indexer versionnumber)
