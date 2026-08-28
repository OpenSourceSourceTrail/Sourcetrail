find_package(Doxygen REQUIRED)
if(DOXYGEN_FOUND)
  set(DOXYGEN_IN ${PROJECT_SOURCE_DIR}/docs/Doxyfile.in)
  set(DOXYGEN_OUT ${PROJECT_BINARY_DIR}/docs/Doxyfile)
  set(DOXYGEN_HTML_IN ${PROJECT_SOURCE_DIR}/docs/header.html.in)
  set(DOXYGEN_HTML_OUT ${PROJECT_BINARY_DIR}/docs/header.html)

  if(NOT EXISTS ${PROJECT_BINARY_DIR}/docs/v2.1.0.tar.gz)
    file(DOWNLOAD https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.1.0.tar.gz
         ${PROJECT_BINARY_DIR}/docs/v2.1.0.tar.gz)
  endif()
  if(NOT EXISTS ${PROJECT_BINARY_DIR}/docs/doxygen-awesome-css-2.1.0)
    file(ARCHIVE_EXTRACT INPUT ${PROJECT_BINARY_DIR}/docs/v2.1.0.tar.gz DESTINATION ${PROJECT_BINARY_DIR}/docs/html)
  endif()

  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  configure_file(${DOXYGEN_HTML_IN} ${DOXYGEN_HTML_OUT} @ONLY)

  add_custom_target(
    doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/docs/
    COMMENT "Generating API documentation with Doxygen")
else()
  message("Doxygen need to be installed to generate the doxygen documentation")
endif()
