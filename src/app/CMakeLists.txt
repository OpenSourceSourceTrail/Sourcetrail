# The executables. See the process model in CLAUDE.md: the GUI owns no database and reaches the
# index through the engine, which is why cli/ and engine/ are Qt-free.
add_subdirectory(gui)
add_subdirectory(cli)
add_subdirectory(engine)
