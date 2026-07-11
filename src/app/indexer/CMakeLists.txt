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

# Advertise the built-in C/C++ indexer as a discoverable plugin so the project wizard
# offers the C/C++ source group types. The actual executable is resolved via
# AppPath::getCxxIndexerFilePath(), so only the manifest is copied (not the binary).
if(BUILD_CXX_LANGUAGE_PACKAGE)
  set(CXX_PLUGIN_DIR "${CMAKE_BINARY_DIR}/app/plugins/cxx")
  add_custom_command(
    TARGET Sourcetrail_indexer
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CXX_PLUGIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/manifest.xml" "${CXX_PLUGIN_DIR}/manifest.xml"
    COMMENT "Copying built-in C/C++ indexer plugin manifest")
endif()

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
