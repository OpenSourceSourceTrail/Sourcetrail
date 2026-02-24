# ${CMAKE_SOURCE_DIR}/src/lib_llm/CMakeLists.txt
add_sourcetrail_interface(
NAME llm::common
DEPS
    Qt6::Core)

add_subdirectory(services)
