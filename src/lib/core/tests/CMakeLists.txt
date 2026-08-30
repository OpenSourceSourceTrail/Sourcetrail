# ${PROJECT_SOURCE_DIR}/src/core/tests/CMakeLists.txt
add_sourcetrail_test(
  NAME ConfigManagerTestSuite
  SOURCES ConfigManagerTestSuite.cpp
  DEPS Sourcetrail::lib::core Sourcetrail::lib::core::utility::ConfigManager Sourcetrail::lib::core::utility::TextAccess
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME UtilityStringTestSuite
  SOURCES UtilityStringTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::utilityString
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME TextAccessTestSuite
  SOURCES TextAccessTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::TextAccess
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME TextCodecTestSuite
  SOURCES TextCodecTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::TextCodec
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME VersionTestSuite
  SOURCES VersionTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::Version
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME UnorderedCacheTestSuite
  SOURCES UnorderedCacheTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::UnorderedCache
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME ScopedFunctorTestSuite
  SOURCES ScopedFunctorTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::ScopedFunctor
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME StatusTestSuite
  SOURCES StatusTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::Status
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME FileManagerTestSuite
  SOURCES FileManagerTestSuite.cpp
  DEPS Sourcetrail::lib::core Sourcetrail::lib::core::utility::utility
       Sourcetrail::lib::core::utility::file::FileManager Sourcetrail::lib::core::utility::file::FilePath
       Sourcetrail::lib::core::utility::file::FilePathFilter Sourcetrail::lib::core::utility::file::FileSystem
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME LowMemoryStringMapTestSuite
  SOURCES LowMemoryStringMapTestSuite.cpp
  DEPS Sourcetrail::lib::core Sourcetrail::lib::core::utility::GlobalId
       Sourcetrail::lib::core::utility::LowMemoryStringMap Sourcetrail::lib::core::utility::TextAccess
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME CommandlineCommandConfigTestSuite
  SOURCES CommandlineCommandConfigTestSuite.cpp
  DEPS lib_test_utilities lib::mocks Sourcetrail::lib::core::utility::commandline::CommandlineCommandConfig
       Sourcetrail::lib::core::utility::commandline::CommandLineParser Sourcetrail::lib::lib
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME CommandlineCommandIndexTestSuite
  SOURCES CommandlineCommandIndexTestSuite.cpp
  DEPS lib_test_utilities lib::mocks Sourcetrail::lib::core::utility::commandline::CommandlineCommandConfig
       Sourcetrail::lib::core::utility::commandline::CommandlineCommandIndex
       Sourcetrail::lib::core::utility::commandline::CommandLineParser Sourcetrail::lib::lib
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME CommandlineCommandTestSuite
  SOURCES CommandlineCommandTestSuite.cpp
  DEPS lib::mocks Sourcetrail::lib::core::utility::commandline::commands::CommandlineCommand
       Sourcetrail::lib::core::utility::commandline::CommandLineParser lib_test_utilities
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME CommandlineHelperTestSuite
  SOURCES CommandlineHelperTestSuite.cpp
  DEPS Boost::program_options Sourcetrail::lib::core::utility::commandline::CommandLineParser
       Sourcetrail::lib::core::utility::commandline::CommandlineHelper Sourcetrail::lib::core::utility::file::FilePath
       lib_test_utilities
  TEST_PREFIX "unittests.core.")

if(TARGET CommandlineHelperTestSuite AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(CommandlineHelperTestSuite PRIVATE -Wno-write-strings)
endif()
add_sourcetrail_test(
  NAME CommandLineParserTestSuite
  SOURCES CommandLineParserTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::commandline::CommandLineParser lib_test_utilities
       Sourcetrail::lib::core::utility::utilityString
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME CommandlineTestSuite
  SOURCES CommandlineTestSuite.cpp
  DEPS lib::mocks Sourcetrail::lib::lib Sourcetrail::lib::core::utility::commandline::CommandLineParser
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME UtilityTestSuite
  SOURCES UtilityTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::utility
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME FilePathFilterTestSuite
  SOURCES FilePathFilterTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::file::FilePathFilter
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME FilePathTestSuite
  SOURCES FilePathTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::file::FilePath
  TEST_PREFIX "unittests.core.")

target_compile_definitions(
  FilePathTestSuite PRIVATE LIB_TEST_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\" ROOT_DIR=\"${PROJECT_SOURCE_DIR}\"
                            SOURCE_PATH_PREFIX_LEN=${SOURCE_PATH_PREFIX_LEN})

add_sourcetrail_test(
  NAME UtilityFileFileSystemTestSuite
  SOURCES UtilityFileFileSystemTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::file::FileSystem Sourcetrail::lib::core::utility::file::FileManager
       Sourcetrail::lib::core::utility::file::FilePathFilter Sourcetrail::lib::core::utility::utility
       Sourcetrail::lib::core::utility::file::ScopedTemporaryFile
  TEST_PREFIX "unittests.core.")

target_compile_definitions(UtilityFileFileSystemTestSuite PRIVATE LIB_TEST_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
                                                                  ROOT_DIR=\"${PROJECT_SOURCE_DIR}\")

add_sourcetrail_test(
  NAME ScopedTemporaryFileTestSuite
  SOURCES ScopedTemporaryFileTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::file::ScopedTemporaryFile
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME FileSystemTestSuite
  SOURCES FileSystemTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::FileSystem
  TEST_PREFIX "unittests.core.")

add_sourcetrail_test(
  NAME utilityAppTestSuite
  SOURCES utilityAppTestSuite.cpp
  DEPS Sourcetrail::lib::core::utility::utilityApp
  TEST_PREFIX "unittests.core.")
