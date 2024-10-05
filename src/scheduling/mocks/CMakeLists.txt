# ${CMAKE_SOURCE_DIR}/src/scheduling/mocks/CMakeLists.txt
add_library(Sourcetrail_scheduling_mocks INTERFACE)
add_library(Sourcetrail::scheduling::mocks ALIAS Sourcetrail_scheduling_mocks)

target_include_directories(Sourcetrail_scheduling_mocks INTERFACE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(Sourcetrail_scheduling_mocks INTERFACE Sourcetrail::scheduling GTest::gmock)
