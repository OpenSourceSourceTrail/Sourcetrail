# ${CMAKE_SOURCE_DIR}/indexers/cxx/CMakeLists.txt
# Built-in C/C++ indexer plugin: the Clang/LibTooling language package plus the worker
# executable that hosts it. Staged into <build>/app/plugins/cxx/ like every other plugin.
# lib must come first -- indexer links the alias target it defines.
add_subdirectory(lib)
add_subdirectory(indexer)
