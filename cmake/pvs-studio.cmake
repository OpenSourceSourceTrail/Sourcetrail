# Find PVS-Studio executables
find_program(PVS_STUDIO_ANALYZER_EXECUTABLE pvs-studio-analyzer)
find_program(PLOG_CONVERTER_EXECUTABLE plog-converter)

include(ProcessorCount)
ProcessorCount(PVS_STUDIO_JOBS)
if(PVS_STUDIO_JOBS EQUAL 0)
  set(PVS_STUDIO_JOBS 1)
endif()

# Path to the opensource license obtained via `pvs-studio-analyzer credentials`
set(PVS_STUDIO_LICENSE_FILE
    "$ENV{HOME}/.config/PVS-Studio/PVS-Studio.lic"
    CACHE FILEPATH "Path to PVS-Studio license file")

if(PVS_STUDIO_ANALYZER_EXECUTABLE)
  # Stage 1: analyze compile_commands.json, produce raw PVS-Studio log
  add_custom_target(
    pvs-studio-analyze
    COMMAND
      ${PVS_STUDIO_ANALYZER_EXECUTABLE} analyze -f ${CMAKE_BINARY_DIR}/compile_commands.json -l
      ${PVS_STUDIO_LICENSE_FILE} -o ${CMAKE_BINARY_DIR}/pvs-studio.log -j ${PVS_STUDIO_JOBS} -e
      ${CMAKE_SOURCE_DIR}/external
    COMMENT "Running pvs-studio-analyzer using compile_commands.json"
    VERBATIM
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json)
else()
  message(WARNING "PVS-Studio not found. Static analysis will be skipped.")
endif()

if(PLOG_CONVERTER_EXECUTABLE)
  # Stage 2: convert raw log to a browsable HTML report
  add_custom_target(
    pvs-studio-report
    COMMAND ${PLOG_CONVERTER_EXECUTABLE} -a GA:1,2,3 -t fullhtml -o
            ${CMAKE_BINARY_DIR}/pvs-studio-report ${CMAKE_BINARY_DIR}/pvs-studio.log
    COMMENT "Converting pvs-studio.log to an HTML report"
    VERBATIM)
  if(PVS_STUDIO_ANALYZER_EXECUTABLE)
    add_dependencies(pvs-studio-report pvs-studio-analyze)
  endif()
else()
  message(WARNING "plog-converter not found. Generation of html report will be skipped.")
endif()

if(PVS_STUDIO_ANALYZER_EXECUTABLE AND PLOG_CONVERTER_EXECUTABLE)
  # Umbrella target: run every stage, end with the HTML report ready to open
  add_custom_target(pvs-studio DEPENDS pvs-studio-report)
endif()
