# ${CMAKE_SOURCE_DIR}/tests/integration/engine_http/CMakeLists.txt
#
# Black-box tests that spawn the real sourcetrail_engine and drive its HTTP API. Python rather
# than GTest on purpose: an independent client shares no code with what it is testing.
#
# add_sourcetrail_test() cannot register these -- it is hardwired to add_executable() plus
# gtest_discover_tests() -- so this uses bare add_test(), the same way the linksNoSqlite guard
# in src/app/gui/CMakeLists.txt does.

find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_Interpreter_FOUND)
  message(STATUS "engine_http tests: skipped, no Python 3 interpreter found")
  return()
endif()

if(NOT TARGET Sourcetrail_engine)
  message(STATUS "engine_http tests: skipped, Sourcetrail_engine is not being built")
  return()
endif()

# One CTest entry per module rather than per test case: unittest has no equivalent of
# gtest_discover_tests, and a handful of entries keeps failures attributable without a
# discovery step.
set(engine_http_modules transport lifecycle queries bookmarks events)

foreach(module IN LISTS engine_http_modules)
  set(test_name "integration.engine_http.${module}")

  add_test(
    NAME ${test_name}
    COMMAND ${Python3_EXECUTABLE} -m unittest -v "test_${module}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

  # The sample projects are read from the source tree and copied into a temp dir by the tests,
  # because loading a project rewrites its .srctrldb and .srctrlbm in place.
  set_tests_properties(
    ${test_name}
    PROPERTIES
      ENVIRONMENT
      "SOURCETRAIL_ENGINE=$<TARGET_FILE:Sourcetrail_engine>;SOURCETRAIL_PROJECTS_DIR=${CMAKE_SOURCE_DIR}/bin/app/user/projects"
      TIMEOUT
      180
      # Each module starts engines on ephemeral ports and several spawn more than one;
      # serialising them keeps a loaded machine from tripping the startup timeouts.
      RUN_SERIAL
      TRUE)
endforeach()
