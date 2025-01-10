# Ensure cppcheck is available
find_program(CPPCHECK_EXECUTABLE cppcheck)

# Check if cppcheck is available
if(CPPCHECK_EXECUTABLE)
  # Cppcheck configuration
  set(CPPCHECK_OPTIONS
      --enable=warning,performance,portability
      --suppress=missingInclude
      --error-exitcode=0
      --std=c++${CMAKE_CXX_STANDARD}
      --verbose
      --project=${CMAKE_BINARY_DIR}/compile_commands.json
      --xml
      --output-file=${CMAKE_BINARY_DIR}/cppcheck.xml)

  # Create a custom target for cppcheck
  add_custom_target(
    cppcheck
    COMMAND ${CPPCHECK_EXECUTABLE} ${CPPCHECK_OPTIONS}
    COMMENT "Running cppcheck using compile_commands.json"
    VERBATIM
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json)
else()
  message(WARNING "Cppcheck not found. Static analysis will be skipped.")
endif()
