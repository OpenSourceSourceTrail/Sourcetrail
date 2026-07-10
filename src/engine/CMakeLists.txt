add_executable(Sourcetrail_engine main.cpp EngineServiceImpl.cpp EngineServiceImpl.h)

set_target_properties(Sourcetrail_engine PROPERTIES
  OUTPUT_NAME sourcetrail_engine
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app/")

target_link_libraries(
  Sourcetrail_engine
  PRIVATE Sourcetrail::core
          Sourcetrail::lib
          Sourcetrail::lib::utility::PathDetection
          Sourcetrail::proto::api
          Sourcetrail::proto::convert
          Sourcetrail::messaging
          gRPC::grpc++
          $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib_cxx>
          Sourcetrail::core::utility::logging
          Sourcetrail::core::utility::ScopedFunctor
          Sourcetrail::core::utility::TextAccess
          Sourcetrail::core::utility::utilityApp)

target_include_directories(
  Sourcetrail_engine
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
          "${CMAKE_BINARY_DIR}/src/lib"
          "${CMAKE_BINARY_DIR}/src/lib_gui")

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail_engine PRIVATE ${CORE_FOUNDATION})
endif()

add_dependencies(Sourcetrail_engine versionnumber)
