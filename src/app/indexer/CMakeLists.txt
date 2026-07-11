#${CMAKE_SOURCE_DIR}/src/app/indexer/CMakeLists.txt
if(UNIX)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app/")
else()
  foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${OUTPUTCONFIG}/app/")
  endforeach()
endif()

add_executable(Sourcetrail_indexer main.cpp)

set_target_properties(Sourcetrail_indexer PROPERTIES OUTPUT_NAME sourcetrail_indexer)

if(WIN32)
  # hide the console when running a release build.
  set_target_properties(
    Sourcetrail_indexer
    PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE /DEBUG:FASTLINK"
               COMPILE_DEFINITIONS_DEBUG "_CONSOLE"
               LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
               COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE"
               LINK_FLAGS_RELEASE "/ENTRY:\"mainCRTStartup\" /SUBSYSTEM:WINDOWS /DEBUG"
               LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")

  target_compile_definitions(Sourcetrail_indexer PRIVATE NOMINMAX)

  # generate pdb for release build
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
endif()

target_link_libraries(
  Sourcetrail_indexer
  PRIVATE $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib::lib_cxx>
          Sourcetrail::lib::core
          Sourcetrail::lib::lib
          Sourcetrail::lib::proto::convert
          Sourcetrail::lib::messaging
          Sourcetrail::lib::core::utility::logging)

if(APPLE)
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(Sourcetrail_indexer PRIVATE ${CORE_FOUNDATION})
endif()

target_include_directories(
  Sourcetrail_indexer
  PRIVATE "${CMAKE_BINARY_DIR}/src/lib/lib"
          "${CMAKE_BINARY_DIR}/src/lib/lib_gui")
