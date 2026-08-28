# Developer convenience targets for running clang-tidy over the tree. CI gates clang-tidy
# separately, on the changed files of a pull request (.github/workflows/clang_tidy.yml).
find_program(CLANG_TIDY_EXECUTABLE clang-tidy)

if(NOT CLANG_TIDY_EXECUTABLE)
  message(STATUS "clang-tidy not found; the clang-tidy targets are not available.")
  return()
endif()

# Only the project's own translation units. The previous glob passed the src directory as the first
# globbing expression and the patterns relative to the source root, so it recursed over build/ and
# .conan/ as well.
file(GLOB_RECURSE SOURCETRAIL_TIDY_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp"
     "${PROJECT_SOURCE_DIR}/indexers/*.cpp")

# clang-tidy reads flags per file from compile_commands.json, which CMAKE_EXPORT_COMPILE_COMMANDS
# writes into the build directory.
add_custom_target(
  clang-tidy
  COMMAND ${CLANG_TIDY_EXECUTABLE} --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy -p ${PROJECT_BINARY_DIR}
          ${SOURCETRAIL_TIDY_SOURCES}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMENT "Running clang-tidy static analysis"
  VERBATIM)

add_custom_target(
  clang-tidy-fix
  COMMAND ${CLANG_TIDY_EXECUTABLE} --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy -p ${PROJECT_BINARY_DIR} -fix
          ${SOURCETRAIL_TIDY_SOURCES}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMENT "Applying clang-tidy fixes"
  VERBATIM)
