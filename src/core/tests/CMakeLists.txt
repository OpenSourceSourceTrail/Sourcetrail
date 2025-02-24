# ${CMAKE_SOURCE_DIR}/src/core/tests/CMakeLists.txt

# ================================================================================================================================
add_executable(ConfigManagerTestSuite ConfigManagerTestSuite.cpp)

target_link_libraries(
  ConfigManagerTestSuite
  PUBLIC Sourcetrail::gtest_main
         Sourcetrail::core
         Sourcetrail::core::utility::ConfigManager
         Sourcetrail::core::utility::TextAccess
         $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
         $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

set_target_properties(ConfigManagerTestSuite PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

gtest_discover_tests(
  ConfigManagerTestSuite
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
  TEST_PREFIX "unittests.core.")
# ================================================================================================================================
add_executable(UtilityStringTestSuite UtilityStringTestSuite.cpp)

target_link_libraries(
  UtilityStringTestSuite
  PUBLIC Sourcetrail::gtest_main
         Sourcetrail::core::utility::utilityString
         $<$<BOOL:${ENABLE_SANITIZER_ADDRESS}>:sanitizer::address>
         $<$<BOOL:${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR}>:sanitizer::undefined>)

set_target_properties(UtilityStringTestSuite PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/")

gtest_discover_tests(
  UtilityStringTestSuite
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/test/"
  TEST_PREFIX "unittests.core.")
# ================================================================================================================================
add_sourcetrail_test(
  NAME
  TextAccessTestSuite
  SOURCES
  TextAccessTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::TextAccess
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  TextCodecTestSuite
  SOURCES
  TextCodecTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::TextCodec
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  VersionTestSuite
  SOURCES
  VersionTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::Version
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  UnorderedCacheTestSuite
  SOURCES
  UnorderedCacheTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::UnorderedCache
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  ScopedFunctorTestSuite
  SOURCES
  ScopedFunctorTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::ScopedFunctor
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  StatusTestSuite
  SOURCES
  StatusTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::Status
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  FileManagerTestSuite
  SOURCES
  FileManagerTestSuite.cpp
  DEPS
  Sourcetrail::core
  Sourcetrail::core::utility::utility
  Sourcetrail::core::utility::file::FileManager
  Sourcetrail::core::utility::file::FilePath
  Sourcetrail::core::utility::file::FilePathFilter
  Sourcetrail::core::utility::file::FileSystem
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  LowMemoryStringMapTestSuite
  SOURCES
  LowMemoryStringMapTestSuite.cpp
  DEPS
  Sourcetrail::core
  Sourcetrail::core::utility::GlobalId
  Sourcetrail::core::utility::LowMemoryStringMap
  Sourcetrail::core::utility::TextAccess
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  CommandlineCommandConfigTestSuite
  SOURCES
  CommandlineCommandConfigTestSuite.cpp
  DEPS
  lib_test_utilities
  lib::mocks
  Sourcetrail::core::utility::commandline::CommandlineCommandConfig
  Sourcetrail::core::utility::commandline::CommandLineParser
  Sourcetrail::lib
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  CommandlineCommandIndexTestSuite
  SOURCES
  CommandlineCommandIndexTestSuite.cpp
  DEPS
  lib_test_utilities
  lib::mocks
  Sourcetrail::core::utility::commandline::CommandlineCommandConfig
  Sourcetrail::core::utility::commandline::CommandlineCommandIndex
  Sourcetrail::core::utility::commandline::CommandLineParser
  Sourcetrail::lib
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  CommandlineCommandTestSuite
  SOURCES
  CommandlineCommandTestSuite.cpp
  DEPS
  lib::mocks
  Sourcetrail::core::utility::commandline::commands::CommandlineCommand
  Sourcetrail::core::utility::commandline::CommandLineParser
  lib_test_utilities
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  CommandlineHelperTestSuite
  SOURCES
  CommandlineHelperTestSuite.cpp
  DEPS
  Boost::program_options
  Sourcetrail::core::utility::commandline::CommandLineParser
  Sourcetrail::core::utility::commandline::CommandlineHelper
  Sourcetrail::core::utility::file::FilePath
  lib_test_utilities
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

if(TARGET CommandlineHelperTestSuite AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(CommandlineHelperTestSuite PRIVATE -Wno-write-strings)
endif()

add_sourcetrail_test(
  NAME
  CommandLineParserTestSuite
  SOURCES
  CommandLineParserTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::commandline::CommandLineParser
  lib_test_utilities
  Sourcetrail::core::utility::utilityString
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  CommandlineTestSuite
  SOURCES
  CommandlineTestSuite.cpp
  DEPS
  lib::mocks
  Sourcetrail::lib
  Sourcetrail::core::utility::commandline::CommandLineParser
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  UtilityTestSuite
  SOURCES
  UtilityTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::utility
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  FilePathFilterTestSuite
  SOURCES
  FilePathFilterTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::file::FilePathFilter
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  FilePathTestSuite
  SOURCES
  FilePathTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::file::FilePath
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

target_compile_definitions(
  FilePathTestSuite PRIVATE LIB_TEST_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\" ROOT_DIR=\"${CMAKE_SOURCE_DIR}\"
                            SOURCE_PATH_PREFIX_LEN=${SOURCE_PATH_PREFIX_LEN})

add_sourcetrail_test(
  NAME
  UtilityFileFileSystemTestSuite
  SOURCES
  UtilityFileFileSystemTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::file::FileSystem
  Sourcetrail::core::utility::file::FileManager
  Sourcetrail::core::utility::file::FilePathFilter
  Sourcetrail::core::utility::utility
  Sourcetrail::core::utility::file::ScopedTemporaryFile
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

target_compile_definitions(UtilityFileFileSystemTestSuite PRIVATE LIB_TEST_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
                                                                  ROOT_DIR=\"${CMAKE_SOURCE_DIR}\")

add_sourcetrail_test(
  NAME
  ScopedTemporaryFileTestSuite
  SOURCES
  ScopedTemporaryFileTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::file::ScopedTemporaryFile
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  FileSystemTestSuite
  SOURCES
  FileSystemTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::FileSystem
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")

add_sourcetrail_test(
  NAME
  loggingTestSuite
  SOURCES
  loggingTestSuite.cpp
  DEPS
  Sourcetrail::core::utility::logging
  TEST_PREFIX
  "unittests.core."
  WORKING_DIRECTORY
  "${CMAKE_BINARY_DIR}/test/")
