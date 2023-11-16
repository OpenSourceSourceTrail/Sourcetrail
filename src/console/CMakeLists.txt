# ${CMAKE_SOURCE_DIR}/src/console/CMakeLists.txt
project(SourcetrailConsole CXX)

add_executable(
        SourcetrailConsole
        main.cpp)

target_link_libraries(
        SourcetrailConsole
        PRIVATE
        Qt5::Core
        fmt::fmt
        Sourcetrail::lib)
