# ${CMAKE_SOURCE_DIR}/mcp/CMakeLists.txt
#
# Registers the MCP server's check. Same shape as tests/integration/engine_http: bare add_test()
# rather than add_sourcetrail_test(), which is hardwired to add_executable() plus
# gtest_discover_tests().

find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_Interpreter_FOUND)
  message(STATUS "mcp test: skipped, no Python 3 interpreter found")
  return()
endif()

if(NOT TARGET Sourcetrail_engine)
  message(STATUS "mcp test: skipped, Sourcetrail_engine is not being built")
  return()
endif()

add_test(
  NAME integration.mcp
  COMMAND ${Python3_EXECUTABLE} -m unittest -v test_mcp
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# The sample project is read from the source tree and copied into a temp dir by the test, because
# loading a project rewrites its .srctrldb and .srctrlbm in place.
set_tests_properties(
  integration.mcp
  PROPERTIES
    ENVIRONMENT
    "SOURCETRAIL_ENGINE=$<TARGET_FILE:Sourcetrail_engine>;SOURCETRAIL_PROJECTS_DIR=${CMAKE_SOURCE_DIR}/bin/app/user/projects"
    TIMEOUT
    180
    # Starts an engine on an ephemeral port; serialising keeps a loaded machine from tripping the
    # startup timeouts.
    RUN_SERIAL
    TRUE)
