# ${CMAKE_SOURCE_DIR}/src/lib/tests/mocks/CMakeLists.txt
add_library(lib_mocks INTERFACE)
add_library(lib::mocks ALIAS lib_mocks)
target_include_directories(lib_mocks INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(lib_mocks INTERFACE GTest::gmock GTest::gtest)
