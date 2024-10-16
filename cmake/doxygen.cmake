find_package(Doxygen)
if(DOXYGEN_FOUND)
  set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/docs/Doxyfile)
  set(DOXYGEN_HTML_IN ${CMAKE_SOURCE_DIR}/docs/header.html.in)
  set(DOXYGEN_HTML_OUT ${CMAKE_BINARY_DIR}/docs/header.html)

  if(NOT EXISTS ${CMAKE_BINARY_DIR}/docs/v2.1.0.tar.gz)
    file(DOWNLOAD https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.1.0.tar.gz
         ${CMAKE_BINARY_DIR}/docs/v2.1.0.tar.gz)
  endif()
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/docs/doxygen-awesome-css-2.1.0)
    file(
      ARCHIVE_EXTRACT
      INPUT
      ${CMAKE_BINARY_DIR}/docs/v2.1.0.tar.gz
      DESTINATION
      ${CMAKE_BINARY_DIR}/docs/html)
  endif()

  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  configure_file(${DOXYGEN_HTML_IN} ${DOXYGEN_HTML_OUT} @ONLY)

  add_custom_target(
    doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/docs/
    COMMENT "Generating API documentation with Doxygen")
else()
  message("Doxygen need to be installed to generate the doxygen documentation")
endif()
