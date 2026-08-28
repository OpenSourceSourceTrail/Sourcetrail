# Fails if BINARY carries any SQLite symbol. Used by guard.gui.linksNoSqlite; see src/app/gui.
execute_process(
  COMMAND "${NM}" --format=posix "${BINARY}"
  OUTPUT_VARIABLE symbols
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "nm failed on ${BINARY}")
endif()

string(REGEX MATCHALL "[^\n]*sqlite3_[^\n]*" hits "${symbols}")
list(LENGTH hits count)
if(count GREATER 0)
  list(SUBLIST hits 0 5 sample)
  message(FATAL_ERROR "${BINARY} references SQLite (${count} symbols). It must reach storage only "
                      "through the engine. First offenders:\n${sample}")
endif()
message(STATUS "${BINARY} is free of SQLite symbols.")
