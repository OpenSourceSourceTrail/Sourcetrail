#${CMAKE_SOURCE_DIR}/src/app/indexer_java_stub/CMakeLists.txt
# Output next to the manifest.xml (plugins/java/) so IndexerPluginRegistry's manifest-relative
# executable resolution finds it, and appears as a discoverable plugin under the app's shared
# data directory (<app dir>/plugins/) without any install step.
set(JAVA_STUB_PLUGIN_DIR "${CMAKE_BINARY_DIR}/app/plugins/java")

if(UNIX)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${JAVA_STUB_PLUGIN_DIR}")
else()
  foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${JAVA_STUB_PLUGIN_DIR}")
  endforeach()
endif()

add_executable(Sourcetrail_indexer_java_stub main.cpp IndexerJavaStub.cpp IndexerJavaStub.h LanguagePackageJavaStub.cpp
                                              LanguagePackageJavaStub.h)

set_target_properties(Sourcetrail_indexer_java_stub PROPERTIES OUTPUT_NAME sourcetrail_indexer_java_stub)

add_custom_command(
  TARGET Sourcetrail_indexer_java_stub
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/manifest.xml" "${JAVA_STUB_PLUGIN_DIR}/manifest.xml"
  COMMENT "Copying Java indexer stub plugin manifest")

if(WIN32)
  target_compile_definitions(Sourcetrail_indexer_java_stub PRIVATE NOMINMAX)
endif()

target_link_libraries(
  Sourcetrail_indexer_java_stub
  PRIVATE Sourcetrail::lib::core
          Sourcetrail::lib::lib
          Sourcetrail::lib::proto::convert
          Sourcetrail::lib::messaging
          Sourcetrail::lib::core::utility::logging)

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail_indexer_java_stub PRIVATE ${CORE_FOUNDATION})
endif()

target_include_directories(Sourcetrail_indexer_java_stub PRIVATE "${CMAKE_BINARY_DIR}/src/lib/lib")
