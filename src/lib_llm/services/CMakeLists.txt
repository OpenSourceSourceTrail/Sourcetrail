# ${CMAKE_SOURCE_DIR}/src/lib_llm/services/CMakeLists.txt
add_sourcetrail_library(
NAME llm::services
SOURCES
    ILLMService.cpp
    # LlmContextBuilder.cpp
    LlmCoordinator.cpp
PUBLIC_HEADERS
    ILLMService.hpp
    # LlmContextBuilder.hpp
    LlmCoordinator.hpp
PUBLIC_DEPS
    Qt6::Core
    nonstd::expected-lite
    Sourcetrail::llm::common
PRIVATE_DEPS
    Qt6::Concurrent
    Sourcetrail::lib)
