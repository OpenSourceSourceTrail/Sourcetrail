# The engine itself, as a library: the daemon binary below is one host for it, the GUI is the other.
add_library(
  Sourcetrail_engine_host STATIC
  EngineEventPublisher.cpp
  EngineEventPublisher.h
  EngineHost.cpp
  EngineHost.h
  EngineHttpService.cpp
  EngineHttpService.h
  GraphLayoutService.cpp
  GraphLayoutService.h)

add_library(Sourcetrail::app::engine_host ALIAS Sourcetrail_engine_host)

target_link_libraries(
  Sourcetrail_engine_host
  PUBLIC Sourcetrail::lib::lib_engine Sourcetrail::lib::proto::api Sourcetrail::lib::core::http
         Sourcetrail::lib::messaging
  PRIVATE Sourcetrail::lib::core
          Sourcetrail::lib::proto::convert
          Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::TextAccess
          Sourcetrail::lib::core::utility::utilityApp
          Sourcetrail::lib::core::utility::utilityUuid
          Sourcetrail::warnings)

target_include_directories(
  Sourcetrail_engine_host
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
  PRIVATE "${PROJECT_BINARY_DIR}/src/lib/lib")

add_executable(Sourcetrail_engine main.cpp)

set_target_properties(Sourcetrail_engine PROPERTIES OUTPUT_NAME sourcetrail_engine RUNTIME_OUTPUT_DIRECTORY
                                                                                   "${PROJECT_BINARY_DIR}/app/")

target_link_libraries(
  Sourcetrail_engine
  PRIVATE Sourcetrail::app::engine_host
          Sourcetrail::lib::core
          Sourcetrail::lib::lib_engine
          Sourcetrail::lib::utility::PathDetection
          Sourcetrail::lib::proto::api
          Sourcetrail::lib::proto::convert
          Sourcetrail::lib::messaging
          Sourcetrail::lib::core::http
          Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::ScopedFunctor
          Sourcetrail::lib::core::utility::TextAccess
          Sourcetrail::lib::core::utility::utilityApp
          Sourcetrail::lib::core::utility::utilityUuid)

# The lib_gui binary dir is here for productVersion.h, which cmake/version.cmake generates into it.
# Not a GUI dependency -- but until that header is generated somewhere neutral, a headless binary
# still has to look in the GUI's build directory for it.
target_include_directories(
  Sourcetrail_engine
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/src/lib/lib" "${PROJECT_BINARY_DIR}/src/lib/lib_gui")

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail_engine PRIVATE ${CORE_FOUNDATION})
endif()

add_dependencies(Sourcetrail_engine versionnumber)

# Project warning set. PRIVATE so it applies here and is not inflicted on consumers.
target_link_libraries(Sourcetrail_engine PRIVATE Sourcetrail::warnings)
