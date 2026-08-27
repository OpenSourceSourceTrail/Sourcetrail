# ${CMAKE_SOURCE_DIR}/tests/integration/CMakeLists.txt
add_subdirectory(engine_http)
add_subdirectory(lib)
add_subdirectory(messaging)
if(BUILD_CXX_LANGUAGE_PACKAGE)
  add_subdirectory(lib_cxx)
endif()
