#include <filesystem>
#include <fstream>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include "FilePath.h"
#include "project/logic/utilitySourceGroupCxx.h"

namespace {

// Each test writes its own fixture tree, so a parallel ctest run cannot have two of them share a
// directory.
class AutoPchFixture : public ::testing::Test {
protected:
  void SetUp() override {
    m_root = FilePath(std::filesystem::temp_directory_path().wstring())
                 .concatenate(FilePath(L"sourcetrail_auto_pch_" + std::to_wstring(reinterpret_cast<uintptr_t>(this))));
    std::filesystem::remove_all(m_root.str());
    std::filesystem::create_directories(m_root.str());
  }

  void TearDown() override {
    std::filesystem::remove_all(m_root.str());
  }

  FilePath write(const std::wstring& name, const std::string& content) {
    const FilePath path = m_root.getConcatenated(name);
    std::filesystem::create_directories(path.getParentDirectory().str());
    std::ofstream out(path.str());
    out << content;
    return path;
  }

  FilePath m_root;
};

TEST_F(AutoPchFixture, KeepsIncludesSharedByEnoughFiles) {
  std::vector<FilePath> sources;
  for(int i = 0; i < 4; i++) {
    sources.push_back(write(L"a" + std::to_wstring(i) + L".cpp", "#include <vector>\n#include <string>\nint main(){}\n"));
  }

  const std::vector<std::string> includes = utility::collectAutoPchIncludes(sources, {}, {});

  EXPECT_EQ(includes, (std::vector<std::string>{"string", "vector"}));
}

TEST_F(AutoPchFixture, DropsIncludesTooFewFilesShare) {
  std::vector<FilePath> sources{write(L"a.cpp", "#include <vector>\n#include <rare>\n"),
                                write(L"b.cpp", "#include <vector>\n"),
                                write(L"c.cpp", "#include <vector>\n"),
                                write(L"d.cpp", "#include <vector>\n")};

  const std::vector<std::string> includes = utility::collectAutoPchIncludes(sources, {}, {});

  EXPECT_EQ(includes, (std::vector<std::string>{"vector"}));
}

TEST_F(AutoPchFixture, IgnoresQuotedIncludes) {
  std::vector<FilePath> sources;
  for(int i = 0; i < 4; i++) {
    sources.push_back(write(L"a" + std::to_wstring(i) + L".cpp", "#include \"local.h\"\n#include <vector>\n"));
  }

  const std::vector<std::string> includes = utility::collectAutoPchIncludes(sources, {}, {});

  EXPECT_EQ(includes, (std::vector<std::string>{"vector"}));
}

TEST_F(AutoPchFixture, DropsIncludesThatResolveIntoTheIndexedSet) {
  // <project/api.h> is the project's own header reached through an -I path: precompiling it would
  // hide its declarations from the indexer's preprocessor callbacks.
  const FilePath ownHeader = write(L"include/project/api.h", "#pragma once\n");
  std::vector<FilePath> sources;
  for(int i = 0; i < 4; i++) {
    sources.push_back(write(L"a" + std::to_wstring(i) + L".cpp", "#include <project/api.h>\n#include <vector>\n"));
  }

  const std::vector<std::string> includes = utility::collectAutoPchIncludes(
      sources, {m_root.getConcatenated(L"include")}, {m_root.getConcatenated(L"include")});

  EXPECT_EQ(includes, (std::vector<std::string>{"vector"}));
  EXPECT_TRUE(ownHeader.exists());
}

TEST_F(AutoPchFixture, DropsIncludesPrecededByAMacroDefinition) {
  // A header whose expansion depends on a macro the source defines first cannot move into a prefix
  // header without changing what it expands to.
  std::vector<FilePath> sources;
  for(int i = 0; i < 4; i++) {
    sources.push_back(
        write(L"a" + std::to_wstring(i) + L".cpp", "#include <vector>\n#define WIN32_LEAN_AND_MEAN\n#include <windows.h>\n"));
  }

  const std::vector<std::string> includes = utility::collectAutoPchIncludes(sources, {}, {});

  EXPECT_EQ(includes, (std::vector<std::string>{"vector"}));
}

TEST_F(AutoPchFixture, ReturnsNothingForATinySourceGroup) {
  std::vector<FilePath> sources{write(L"a.cpp", "#include <vector>\n"), write(L"b.cpp", "#include <vector>\n")};

  EXPECT_TRUE(utility::collectAutoPchIncludes(sources, {}, {}).empty());
}

TEST_F(AutoPchFixture, WritesAPrefixHeaderOfTheGivenIncludes) {
  const FilePath header = utility::writeAutoPchHeader({"vector", "QtCore/QString"}, m_root);

  ASSERT_TRUE(header.exists());
  std::ifstream in(header.str());
  const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_EQ(content, "#include <vector>\n#include <QtCore/QString>\n");
}

TEST_F(AutoPchFixture, WritesNoHeaderWithoutIncludes) {
  EXPECT_TRUE(utility::writeAutoPchHeader({}, m_root).empty());
}

}    // namespace
