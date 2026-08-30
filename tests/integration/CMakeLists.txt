# ${PROJECT_SOURCE_DIR}/tests/integration/CMakeLists.txt
if(ENABLE_ENGINE_HTTP_TEST)
  add_subdirectory(engine_http)
endif()
add_subdirectory(lib)
add_subdirectory(messaging)
if(BUILD_CXX_LANGUAGE_PACKAGE)
  add_subdirectory(lib_cxx)
endif()
