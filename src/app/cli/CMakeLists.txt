#${PROJECT_SOURCE_DIR}/src/app/cli/CMakeLists.txt
add_executable(Sourcetrail_cli main.cpp ConsoleApplication.cpp ConsoleApplication.h)

set_target_properties(Sourcetrail_cli PROPERTIES OUTPUT_NAME sourcetrail_cli RUNTIME_OUTPUT_DIRECTORY
                                                                             "${PROJECT_BINARY_DIR}/app/")

target_link_libraries(
  Sourcetrail_cli
  PRIVATE Sourcetrail::lib::core
          Sourcetrail::lib::lib_engine
          Sourcetrail::lib::proto::convert
          Sourcetrail::lib::utility::PathDetection
          Sourcetrail::lib::messaging
          Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::ScopedFunctor
          Sourcetrail::lib::core::utility::utilityApp
          Sourcetrail::lib::core::utility::commandline::CommandLineParser)

# The lib_gui binary dir is here for productVersion.h, which cmake/version.cmake generates into it.
# Not a GUI dependency -- but until that header is generated somewhere neutral, a headless binary
# still has to look in the GUI's build directory for it.
target_include_directories(Sourcetrail_cli PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/src/lib/lib"
                                                   "${PROJECT_BINARY_DIR}/src/lib/lib_gui")

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail_cli PRIVATE ${CORE_FOUNDATION})
endif()

add_dependencies(Sourcetrail_cli versionnumber)

# Project warning set. PRIVATE so it applies here and is not inflicted on consumers.
target_link_libraries(Sourcetrail_cli PRIVATE Sourcetrail::warnings)
