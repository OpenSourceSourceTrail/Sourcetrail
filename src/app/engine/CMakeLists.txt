add_executable(
  Sourcetrail_engine
  main.cpp
  EngineEventPublisher.cpp
  EngineEventPublisher.h
  EngineHttpService.cpp
  EngineHttpService.h
  GraphLayoutService.cpp
  GraphLayoutService.h)

set_target_properties(Sourcetrail_engine PROPERTIES OUTPUT_NAME sourcetrail_engine RUNTIME_OUTPUT_DIRECTORY
                                                                                   "${PROJECT_BINARY_DIR}/app/")

target_link_libraries(
  Sourcetrail_engine
  PRIVATE Sourcetrail::lib::core
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

target_include_directories(Sourcetrail_engine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/src/lib/lib"
                                                      "${PROJECT_BINARY_DIR}/src/lib/lib_gui")

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail_engine PRIVATE ${CORE_FOUNDATION})
endif()

add_dependencies(Sourcetrail_engine versionnumber)

# Project warning set. PRIVATE so it applies here and is not inflicted on consumers.
target_link_libraries(Sourcetrail_engine PRIVATE Sourcetrail::warnings)
