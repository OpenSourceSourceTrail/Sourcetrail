# Ensure cppcheck is available
find_program(CPPCHECK_EXECUTABLE cppcheck)
find_program(CPPCHECK_HTMLREPORT_EXECUTABLE cppcheck-htmlreport)

# Check if cppcheck is available
if(CPPCHECK_EXECUTABLE)
  # Cppcheck configuration
  set(CPPCHECK_OPTIONS
      --enable=warning,performance,portability
      --suppress=missingInclude
      --error-exitcode=1
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

if(CPPCHECK_HTMLREPORT_EXECUTABLE)
  # Create a custom target for cppcheck
  add_custom_target(
    cppcheck-htmlreport
    COMMAND ${CPPCHECK_HTMLREPORT_EXECUTABLE} --file=${CMAKE_BINARY_DIR}/cppcheck.xml
            --report-dir=${CMAKE_BINARY_DIR}/cppcheck-report.html --source-dir=${CMAKE_SOURCE_DIR}
    COMMENT "Running cppcheck-htmlreport using cppcheck.xml"
    VERBATIM
    DEPENDS ${CMAKE_BINARY_DIR}/cppcheck.xml)
else()
  message(WARNING "Cppcheck-htmlreport not found. Generation of html report will be skipped.")
endif()
