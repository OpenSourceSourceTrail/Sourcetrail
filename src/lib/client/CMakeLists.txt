# ${CMAKE_SOURCE_DIR}/src/lib/client/CMakeLists.txt
#
# The client's side of the engine boundary. Deliberately free of storage and SQLite: this is what a
# GUI links instead of talking to a database itself.
add_library(Sourcetrail_client)
add_library(Sourcetrail::lib::client ALIAS Sourcetrail_client)

target_sources(
  Sourcetrail_client
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Capabilities.cpp
          ${CMAKE_CURRENT_LIST_DIR}/Capabilities.h
          ${CMAKE_CURRENT_LIST_DIR}/ClientFactory.cpp
          ${CMAKE_CURRENT_LIST_DIR}/ClientFactory.h
          ${CMAKE_CURRENT_LIST_DIR}/EngineCall.h
          ${CMAKE_CURRENT_LIST_DIR}/EngineChannel.cpp
          ${CMAKE_CURRENT_LIST_DIR}/EngineChannel.h
          ${CMAKE_CURRENT_LIST_DIR}/GrpcProject.cpp
          ${CMAKE_CURRENT_LIST_DIR}/GrpcProject.h
          ${CMAKE_CURRENT_LIST_DIR}/GrpcStorageAccess.cpp
          ${CMAKE_CURRENT_LIST_DIR}/GrpcStorageAccess.h)

target_include_directories(Sourcetrail_client PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(
  Sourcetrail_client
  PUBLIC Sourcetrail::lib::lib # StorageAccess and the domain types it returns
         # EngineChannel.h exposes the stub, so its headers reach whoever includes it.
         Sourcetrail::lib::proto::api
         gRPC::grpc++
         Sourcetrail::lib::messaging # ClientFactory hands out the concrete MessageQueue
  PRIVATE Sourcetrail::lib::proto::convert
          Sourcetrail::lib::core
          Sourcetrail::lib::core::utility::logging
          Sourcetrail::lib::core::utility::TextAccess)

if(ENABLE_UNIT_TEST)
  add_subdirectory(tests)
endif()
