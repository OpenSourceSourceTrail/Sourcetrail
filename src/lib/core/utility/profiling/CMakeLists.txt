# ${PROJECT_SOURCE_DIR}/src/lib/core/utility/profiling/CMakeLists.txt
#
# Header-only on purpose: with ENABLE_TRACY off this has to leave nothing behind, and an INTERFACE
# target that links no TracyClient does exactly that.
add_sourcetrail_interface(NAME lib::core::utility::profiling DEPS ${SR_TRACY_DEPS})

if(ENABLE_TRACY)
  target_compile_definitions(Sourcetrail_lib_core_utility_profiling INTERFACE SR_TRACY_ENABLED=1)
endif()
