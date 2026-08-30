# The libraries. Declaration order is not significant: targets are resolved when the build system is
# generated, not as each directory is read.
add_subdirectory(external)
add_subdirectory(proto)
add_subdirectory(lib)
add_subdirectory(client)
add_subdirectory(lib_qml)

add_subdirectory(core)
add_subdirectory(scheduling)
add_subdirectory(messaging)

# utilityApp sits under core/utility/ but is not one of the subdirectories core/utility/CMakeLists.txt
# adds; it is declared here instead.
add_subdirectory(core/utility/utilityApp)
