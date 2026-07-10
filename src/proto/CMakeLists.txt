set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(PROTO_FILES
    "${PROTO_DIR}/sourcetrail_common.proto"
    "${PROTO_DIR}/indexer_worker.proto"
    "${PROTO_DIR}/engine.proto"
)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Generate .pb.h / .pb.cc
protobuf_generate(
  LANGUAGE cpp
  OUT_VAR PROTO_SRCS
  PROTOS ${PROTO_FILES}
  IMPORT_DIRS "${PROTO_DIR}"
  PROTOC_OUT_DIR "${GENERATED_DIR}")

# Generate .grpc.pb.h / .grpc.pb.cc
protobuf_generate(
  LANGUAGE grpc
  PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  OUT_VAR GRPC_SRCS
  PROTOS "${PROTO_DIR}/indexer_worker.proto" "${PROTO_DIR}/engine.proto"
  IMPORT_DIRS "${PROTO_DIR}"
  PROTOC_OUT_DIR "${GENERATED_DIR}")

add_library(Sourcetrail_proto_api STATIC ${PROTO_SRCS} ${GRPC_SRCS})
add_library(Sourcetrail::proto::api ALIAS Sourcetrail_proto_api)

target_include_directories(Sourcetrail_proto_api PUBLIC "${GENERATED_DIR}")

target_link_libraries(
  Sourcetrail_proto_api
  PUBLIC gRPC::grpc++
         protobuf::libprotobuf)

# Domain ↔ proto mapper library (depends on lib domain types declared later)
add_library(Sourcetrail_proto_convert STATIC Convert.cpp Convert.h)
add_library(Sourcetrail::proto::convert ALIAS Sourcetrail_proto_convert)

target_include_directories(Sourcetrail_proto_convert PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# proto_convert needs lib headers to compile but must NOT create a CMake link
# dep on Sourcetrail_lib (that would cause a cycle: lib→convert→lib).
# Reading INTERFACE_INCLUDE_DIRECTORIES via generator expression gives us the
# include paths at generation time without registering a link dependency.
target_include_directories(
  Sourcetrail_proto_convert
  PRIVATE "$<TARGET_PROPERTY:Sourcetrail_lib,INTERFACE_INCLUDE_DIRECTORIES>"
          "$<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:$<TARGET_PROPERTY:Sourcetrail_lib_cxx,INTERFACE_INCLUDE_DIRECTORIES>>")

target_link_libraries(
  Sourcetrail_proto_convert
  PUBLIC Sourcetrail_proto_api
  PRIVATE Sourcetrail::core::utility::logging
          Sourcetrail::core::utility::utilityString)
