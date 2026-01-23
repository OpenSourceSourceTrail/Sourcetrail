# ${CMAKE_SOURCE_DIR}/src/llm/CMakeLists.txt
add_library(
    ILLMService
    STATIC
    ILLMService.cpp)

target_link_libraries(
    ILLMService
    PUBLIC
    Qt6::Core)

target_include_directories(
    ILLMService
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR})
