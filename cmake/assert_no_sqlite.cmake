# Fails if BINARY carries any SQLite symbol. Used by guard.gui.linksNoSqlite; see src/lib/lib_gui.
#
# The companion to cmake/assert_tier_deps.cmake: that one asserts on the declared link graph, this
# one on what the compiler actually emitted. A single #include of a storage header pulls SQLite
# into the archive and is otherwise invisible, so assert on the symbol table rather than on
# anyone's discipline. An undefined `U sqlite3_*` in a static archive is as damning as a defined
# one -- it means this tier calls SQLite.
execute_process(
  COMMAND "${NM}" --format=posix "${BINARY}"
  OUTPUT_VARIABLE symbols
  RESULT_VARIABLE result
  ERROR_QUIET)
if(NOT
   result
   EQUAL
   0)
  message(FATAL_ERROR "nm failed on ${BINARY}")
endif()

string(
  REGEX MATCHALL
        "[^\n]*sqlite3_[^\n]*"
        hits
        "${symbols}")
list(LENGTH hits count)
if(count GREATER 0)
  list(LENGTH hits total)
  if(total GREATER 5)
    set(total 5)
  endif()
  list(
    SUBLIST
    hits
    0
    ${total}
    sample)
  string(REPLACE ";" "\n  " sample "${sample}")
  message(FATAL_ERROR "${BINARY} references SQLite (${count} symbols). The Presentation tier must "
                      "reach storage only through StorageAccess. First offenders:\n  ${sample}")
endif()
message(STATUS "${BINARY} is free of SQLite symbols.")
