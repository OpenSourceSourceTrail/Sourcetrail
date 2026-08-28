# ${CMAKE_SOURCE_DIR}/src/lib/core/http/CMakeLists.txt
#
# Loopback HTTP transport for the client <-> engine boundary. Qt-free and storage-free on purpose:
# both the engine daemon and the GUI's client library link it.
add_sourcetrail_library(
  NAME lib::core::http
  SOURCES HttpServer.cpp HttpClient.cpp
  HEADERS HttpServer.h HttpClient.h
  PUBLIC_DEPS Boost::boost
  PRIVATE_DEPS Boost::system Sourcetrail::lib::core::utility::logging)

if(ENABLE_UNIT_TEST)
  add_subdirectory(tests)
endif()
