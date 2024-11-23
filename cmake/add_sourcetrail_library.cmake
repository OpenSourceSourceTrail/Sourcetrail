function(add_sourcetrail_library)
  # Define the expected arguments
  set(options "")
  # Base name of the library
  set(oneValueArgs NAME)
  set(multiValueArgs
      SOURCES # Source files
      PRIVATE_HEADERS # Private header files
      PUBLIC_HEADERS # Public header files
      PUBLIC_DEPS # Public dependencies
      PRIVATE_DEPS # Private dependencies
  )

  # Parse the arguments
  cmake_parse_arguments(
    ARG
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN})

  # Validate required arguments
  if(NOT DEFINED ARG_NAME)
    message(FATAL_ERROR "NAME argument is required")
  endif()

  # Create the actual library name with the full namespace
  string(
    REPLACE "::"
            "_"
            LIBRARY_NAME
            "Sourcetrail_${ARG_NAME}")

  # Add the library
  add_library(${LIBRARY_NAME})

  # Create the aliased target name with proper namespacing
  string(
    REPLACE "_"
            "::"
            ALIAS_NAME
            "Sourcetrail::${ARG_NAME}")
  add_library(${ALIAS_NAME} ALIAS ${LIBRARY_NAME})

  # Add sources
  target_sources(
    ${LIBRARY_NAME}
    PRIVATE ${ARG_SOURCES} ${ARG_PRIVATE_HEADERS}
    PUBLIC ${ARG_PUBLIC_HEADERS})

  # Set include directories
  target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

  # Add dependencies
  if(ARG_PUBLIC_DEPS)
    target_link_libraries(${LIBRARY_NAME} PUBLIC ${ARG_PUBLIC_DEPS})
  endif()

  if(ARG_PRIVATE_DEPS)
    target_link_libraries(${LIBRARY_NAME} PRIVATE ${ARG_PRIVATE_DEPS})
  endif()

  myproject_set_project_warnings(
    ${LIBRARY_NAME}
    ${SOURCETRAIL_WARNING_AS_ERROR}
    ""
    ""
    ""
    "")
endfunction()
