# Find clang-tidy executable
find_program(CLANG_TIDY_EXECUTABLE clang-tidy)

# Check if clang-tidy is available
if(CLANG_TIDY_EXECUTABLE)
  file(
    GLOB_RECURSE
    SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src
    "*.h"
    "*.hpp"
    "*.cpp")
  # Create a custom target for clang-tidy
  add_custom_target(
    clang-tidy
    COMMAND
      ${CLANG_TIDY_EXECUTABLE}
      # Use default .clang-tidy from project root
      -p ${CMAKE_BINARY_DIR}
      # Optionally specify files or use glob
      # [src/*.cpp include/*.hpp]
      ${SOURCE_FILES}
    COMMAND # Optional: Convert fixes to patch file
            clang-apply-replacements ${CMAKE_BINARY_DIR}/clang-tidy-fixes.yml -format=yaml -style=file ||
            true # Continue even if no fixes found
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy static analysis"
    VERBATIM)

  # Optional: Add a fix target to automatically apply suggested fixes
  add_custom_target(
    clang-tidy-fix
    COMMAND ${CLANG_TIDY_EXECUTABLE} -p ${CMAKE_BINARY_DIR} -fix ${SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Applying clang-tidy fixes"
    VERBATIM)
else()
  message(WARNING "Clang-Tidy not found. Static analysis will be skipped.")
endif()
