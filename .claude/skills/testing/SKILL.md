---
name: testing
description: Build and run Sourcetrail tests — ctest, GTest filters, test locations, add_sourcetrail_test. Use when writing, running, or debugging tests.
---

# Testing

## Enable & build

Configure with `ENABLE_UNIT_TEST=ON` (and/or `ENABLE_INTEGRATION_TEST=ON`), then:

```bash
cmake --preset=gnu_release_build_cxx
cd ../build/gnu_release_build_cxx
ninja
```

## Run

```bash
# All tests
ctest

# A specific test binary directly
./test/<test_binary_name>

# A specific GTest filter
./test/<test_binary_name> --gtest_filter="SuiteName.TestName"
```

## Where tests live

- `src/lib/lib/tests/` — core business logic
- `src/lib/lib_qml/tests/` — GUI
- `indexers/cxx/lib/tests/` — C/C++ language package
- `tests/` — integration tests

New test targets use `add_sourcetrail_test()` (see `cmake/add_sourcetrail_test.cmake`).
