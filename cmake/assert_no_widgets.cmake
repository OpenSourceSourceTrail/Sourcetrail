# Fails if BINARY links Qt Widgets. Used by guard.gui.linksNoWidgets; see src/app/qml_gui.
#
# The inverse of the guard the widget GUI carried: that one kept SQLite out of a process that had to
# reach the index over HTTP. The QML GUI owns the index on purpose, so the thing worth asserting now
# is that nobody quietly reintroduces a QWidget to get a dialog up.
execute_process(
  COMMAND "${NM}" --format=posix --dynamic --undefined-only "${BINARY}"
  OUTPUT_VARIABLE symbols
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "nm failed on ${BINARY}")
endif()

string(REGEX MATCHALL "[^\n]*QWidget[^\n]*" hits "${symbols}")
list(LENGTH hits count)
if(count GREATER 0)
  list(LENGTH hits total)
  if(total GREATER 5)
    list(SUBLIST hits 0 5 sample)
  else()
    set(sample "${hits}")
  endif()
  message(FATAL_ERROR "${BINARY} references Qt Widgets (${count} symbols). The QML front end must "
                      "not depend on the widget module. First offenders:\n${sample}")
endif()
message(STATUS "${BINARY} is free of Qt Widgets symbols.")
