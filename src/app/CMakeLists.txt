# The executables. See the process model in CLAUDE.md: the QML GUI links the engine as a library and
# owns the index; cli/ and engine/ are the Qt-free headless front ends onto the same libraries.
add_subdirectory(qml_gui)
add_subdirectory(cli)
add_subdirectory(engine)
