#include <codecvt>
#include <filesystem>
#include <fstream>
#include <set>

#include <boost/range/algorithm/count.hpp>

#include <gtest/gtest.h>

#include "FilePathFilter.h"
#include "project/logic/utilitySourceGroupCxx.h"


namespace {

// ============================================================================
// Real-World MSVC Compiler Command
// ============================================================================

class WindowsFlagConverterTest : public ::testing::Test {
protected:
  // Helper to check if a flag exists in the result
  void ExpectContains(const std::vector<std::wstring>& args, const std::wstring& flag) {
    EXPECT_NE(std::ranges::find(args, flag), args.end())
        << "Expected to find flag: " << std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(flag);
  }

  // Helper to check if a flag does NOT exist in the result
  void ExpectNotContains(const std::vector<std::wstring>& args, const std::wstring& flag) {
    EXPECT_EQ(std::ranges::find(args, flag), args.end())
        << "Expected NOT to find flag: " << std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(flag);
  }

  // Helper to verify exact conversion
  void ExpectConverts(const std::wstring& input, const std::vector<std::wstring>& expectedOutput) {
    std::vector<std::wstring> args = {L"compiler", input};
    utility::convertWindowsStyleFlagsToUnixStyleFlags(args);

    ASSERT_EQ(args.size(), 1 + expectedOutput.size()) << "Incorrect output size";
    EXPECT_EQ(args[0], L"compiler") << "Executable name should be preserved";

    for(size_t i = 0; i < expectedOutput.size(); ++i) {
      EXPECT_EQ(args[i + 1], expectedOutput[i]) << "Mismatch at position " << i;
    }
  }

  // Helper to count occurrences
  size_t CountOccurrences(const std::vector<std::wstring>& args, const std::wstring& flag) {
    return static_cast<size_t>(boost::range::count(args, flag));
  }
};

// ============================================================================
// REAL WORLD INTEGRATION TEST
// ============================================================================

TEST_F(WindowsFlagConverterTest, RealWorldMsvcCommand) {
  std::vector<std::wstring> args{
      L"cl.exe",
      L"/nologo",
      L"/TP",
      L"-DBOOST_ALL_NO_LIB",
      L"-DBUILD_TYPE=\"\"",
      L"-DD_WINDOWS",
      L"-DFMT_STRING_ALIAS=1",
      L"-DFMT_UNICODE=0",
      L"-DSOURCE_PATH_PREFIX_LEN=52",
      L"-DSPDLOG_COMPILED_LIB",
      L"-DSPDLOG_FMT_EXTERNAL",
      L"-DSPDLOG_WCHAR_TO_UTF8_SUPPORT",
      L"-DTRACY_ENABLE=1",
      L"-Isrc\\core\\utility\\commandline\\commandlineHelper",
      L"-Isrc\\core\\utility\\commandline\\commandLineParser",
      L"-external:I.conan2\\p\\range0301bf3d76d5d\\p\\include",
      L"-external:I.conan2\\p\\b\\spdlo1b49ab0f41a22\\p\\include",
      L"-external:I.conan2\\p\\b\\fmt2ae127ae73cf1\\p\\include",
      L"-external:Isrc\\external\\sqlite",
      L"-external:I.conan2\\p\\b\\sqlit1eed5c4e3f819\\p\\include",
      L"-external:I.conan2\\p\\expec6daf272f385a9\\p\\include",
      L"-external:W0",
      L"/DWIN32",
      L"/D_WINDOWS",
      L"/EHsc",
      L"/Zi",
      L"/O2",
      L"/Ob1",
      L"/DNDEBUG",
      L"-std:c++20",
      L"-MD",
      L"/W4",
      L"/w14242",
      L"/w14254",
      L"/w14263",
      L"/permissive-",
      L"/Zc:preprocessor",
      L"/FoCMakeFiles\\output.obj",
      L"/FdCMakeFiles\\output.pdb",
      L"/FS",
      L"-c",
      LR"(src\core\utility\commandline\commands\commandlineCommandConfig\CommandlineCommandConfig.cpp")"};

  utility::convertWindowsStyleFlagsToUnixStyleFlags(args);

  // Verify executable preserved
  EXPECT_EQ(args[0], L"cl.exe");

  // Verify skipped flags are gone
  ExpectNotContains(args, L"-std:c++20");
  ExpectNotContains(args, L"/nologo");
  ExpectNotContains(args, L"/Zi");
  ExpectNotContains(args, L"/FS");
  ExpectNotContains(args, L"-MD");
  ExpectNotContains(args, L"/FdCMakeFiles\\output.pdb");
  ExpectNotContains(args, L"-external:I");
  ExpectNotContains(args, L"-external:W0");
  ExpectNotContains(args, L"-o");
  ExpectNotContains(args, L"CMakeFiles\\output.obj");
  ExpectNotContains(args, L"-TP");
  ExpectNotContains(args, L"/W4");
  ExpectNotContains(args, L"/w14242");
  ExpectNotContains(args, L"/w14254");
  ExpectNotContains(args, L"/w14263");

  // Verify converted flags
  ExpectContains(args, L"-std=c++20");
  ExpectContains(args, L"-DBOOST_ALL_NO_LIB");
  ExpectContains(args, L"-DWIN32");
  ExpectContains(args, L"-D_WINDOWS");
  ExpectContains(args, L"-DNDEBUG");
  ExpectContains(args, L"-DBOOST_ALL_NO_LIB");
  ExpectContains(args, L"-DBUILD_TYPE=\"\"");
  ExpectContains(args, L"-DD_WINDOWS");
  ExpectContains(args, L"-DFMT_STRING_ALIAS=1");
  ExpectContains(args, L"-DFMT_UNICODE=0");
  ExpectContains(args, L"-DSOURCE_PATH_PREFIX_LEN=52");
  ExpectContains(args, L"-DSPDLOG_COMPILED_LIB");
  ExpectContains(args, L"-DSPDLOG_FMT_EXTERNAL");
  ExpectContains(args, L"-DSPDLOG_WCHAR_TO_UTF8_SUPPORT");
  ExpectContains(args, L"-DTRACY_ENABLE=1");
  ExpectContains(args, L"-DNDEBUG");

  ExpectContains(args, L"-isystem.conan2\\p\\range0301bf3d76d5d\\p\\include");
  ExpectContains(args, L"-isystem.conan2\\p\\b\\spdlo1b49ab0f41a22\\p\\include");
  ExpectContains(args, L"-isystemsrc\\external\\sqlite");

  ExpectContains(args, LR"(src\core\utility\commandline\commands\commandlineCommandConfig\CommandlineCommandConfig.cpp")");
}

}    // namespace

namespace {

// ============================================================================
// Compilation-database path derivation
//
// The two wizard pages used to derive these paths themselves, differently: one canonicalized every
// source file, the other deduplicated the directories first; one applied the exclude filters, the
// other did not. These are the shared versions.
// ============================================================================

/** A real directory tree, because every derivation step calls exists() or getCanonical(). */
class CdbPathsFix : public ::testing::Test {
protected:
  void SetUp() override {
    mRoot = FilePath((std::filesystem::temp_directory_path() /
                      ("cdbPaths" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) +
                       std::to_string(reinterpret_cast<std::uintptr_t>(this))))
                         .wstring());
    std::filesystem::create_directories(mRoot.getConcatenated(L"/src/sub").str());
    std::filesystem::create_directories(mRoot.getConcatenated(L"/include").str());
  }

  void TearDown() override {
    std::error_code ignored;
    std::filesystem::remove_all(mRoot.str(), ignored);
  }

  FilePath touch(const std::wstring& relative) {
    const FilePath path = mRoot.getConcatenated(relative);
    std::ofstream{path.str()} << "\n";
    return path;
  }

  FilePath mRoot;
};

TEST_F(CdbPathsFix, filterKeepsExistingUnexcludedFiles) {
  const FilePath kept = touch(L"/src/kept.cpp");
  EXPECT_EQ(utility::filterCdbSourceFiles({kept}, {}), std::set<FilePath>{kept});
}

TEST_F(CdbPathsFix, filterDropsNonExistentFiles) {
  EXPECT_TRUE(utility::filterCdbSourceFiles({mRoot.getConcatenated(L"/src/ghost.cpp")}, {}).empty());
}

TEST_F(CdbPathsFix, filterDropsExcludedFiles) {
  const FilePath excluded = touch(L"/src/excluded.cpp");
  EXPECT_TRUE(utility::filterCdbSourceFiles({excluded}, {FilePathFilter(L"**/excluded.cpp")}).empty());
}

TEST_F(CdbPathsFix, filterOnEmptyInputIsEmpty) {
  EXPECT_TRUE(utility::filterCdbSourceFiles({}, {}).empty());
}

TEST_F(CdbPathsFix, headerRootsCollapseSourceFilesToTheirDirectory) {
  const std::vector<FilePath> sources{touch(L"/src/a.cpp"), touch(L"/src/b.cpp")};
  EXPECT_EQ(utility::deriveCdbHeaderRoots(sources, {}), std::vector<FilePath>{mRoot.getConcatenated(L"/src").getCanonical()});
}

TEST_F(CdbPathsFix, headerRootsAbsorbNestedDirectoriesIntoTheirParent) {
  const std::vector<FilePath> sources{touch(L"/src/a.cpp"), touch(L"/src/sub/b.cpp")};
  EXPECT_EQ(utility::deriveCdbHeaderRoots(sources, {}), std::vector<FilePath>{mRoot.getConcatenated(L"/src").getCanonical()});
}

TEST_F(CdbPathsFix, headerRootsDropNonExistentIncludePaths) {
  EXPECT_TRUE(utility::deriveCdbHeaderRoots({}, {mRoot.getConcatenated(L"/no-such-include")}).empty());
}

TEST_F(CdbPathsFix, headerRootsKeepExistingIncludePaths) {
  const FilePath include = mRoot.getConcatenated(L"/include");
  EXPECT_EQ(utility::deriveCdbHeaderRoots({}, {include}), std::vector<FilePath>{include.getCanonical()});
}

TEST_F(CdbPathsFix, headerRootsAreDerivedFromUnfilteredSources) {
  // The header roots come from every file the database names -- excluding a source file from
  // indexing does not mean its directory holds no headers worth indexing.
  const std::vector<FilePath> sources{touch(L"/src/a.cpp"), touch(L"/include/b.cpp")};
  EXPECT_EQ(utility::deriveCdbHeaderRoots(sources, {}).size(), 2U);
}

}    // namespace
