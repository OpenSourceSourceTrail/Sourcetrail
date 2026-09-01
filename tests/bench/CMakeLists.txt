# ${PROJECT_SOURCE_DIR}/tests/bench/CMakeLists.txt
# Not a ctest: it needs an indexed database and reports numbers, so it is run by hand and by
# scripts/bench_queries.py. Built with the tests so it cannot rot unnoticed.
add_executable(Sourcetrail_bench bench_storage_access.cpp)

target_link_libraries(
  Sourcetrail_bench
  PRIVATE Sourcetrail::lib::core Sourcetrail::lib::lib Sourcetrail::lib::lib_engine Sourcetrail::lib::client
          Sourcetrail::lib::core::utility::file::FilePath Sourcetrail::warnings)

set_target_properties(Sourcetrail_bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app/")
