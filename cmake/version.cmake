# ${CMAKE_SOURCE_DIR}/cmake/version.cmake
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  find_package(Git)

  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_STRING
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_RESULT)

  # Check if the Git command was successful
  if(NOT
     GIT_RESULT
     EQUAL
     0)
    message(
      WARNING "Failed to run 'git describe' command. Git might not be installed or the repository is not initialized.")
    set(VERSION_MAJOR "1")
    set(VERSION_MINOR "0")
    set(VERSION_PATCH "0")
    set(VERSION_STRING "1.0.0")
  else()
    string(
      REGEX
      REPLACE "^([0-9]+)\\..*"
              "\\1"
              VERSION_MAJOR
              "${VERSION_STRING}")
    string(
      REGEX
      REPLACE "^[0-9]+\\.([0-9]+).*"
              "\\1"
              VERSION_MINOR
              "${VERSION_STRING}")
    string(
      REGEX
      REPLACE "^[0-9]+\\.[0-9]+.([0-9]+)"
              "\\1"
              VERSION_PATCH
              "${VERSION_STRING}")
  endif()
endif()

if(SOURCETRAIL_CMAKE_VERBOSE)
  message(STATUS "Version: ${VERSION_STRING}")
  message(STATUS "Version year: ${VERSION_MAJOR}")
  message(STATUS "Version minor: ${VERSION_MINOR}")
  message(STATUS "Version commit: ${VERSION_PATCH}")
endif()
