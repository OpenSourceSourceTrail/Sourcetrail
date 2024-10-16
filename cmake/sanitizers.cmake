add_library(sanitizer_address INTERFACE)
add_library(sanitizer::address ALIAS sanitizer_address)
target_compile_options(sanitizer_address INTERFACE -fsanitize=address)
target_link_libraries(sanitizer_address INTERFACE -fsanitize=address)

add_library(sanitizer_undefined INTERFACE)
add_library(sanitizer::undefined ALIAS sanitizer_undefined)
target_compile_options(sanitizer_undefined INTERFACE -fsanitize=undefined)
target_link_libraries(sanitizer_undefined INTERFACE -fsanitize=undefined)
