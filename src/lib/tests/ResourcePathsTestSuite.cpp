#include <filesystem>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AppPath.h"
#include "ResourcePaths.h"

namespace fs = std::filesystem;

struct ResourcePathsFix : public testing::Test {
  void SetUp() override {
    AppPath::setSharedDataDirectoryPath(FilePath{fs::temp_directory_path()});
  }
};

TEST_F(ResourcePathsFix, goodCase) {
  const auto tempDir = fs::path{fs::temp_directory_path(), fs::path::path::generic_format};
  ASSERT_EQ(tempDir / "data/color_schemes/", ResourcePaths::getColorSchemesDirectoryPath().str());
  ASSERT_EQ(tempDir / "data/syntax_highlighting_rules/", ResourcePaths::getSyntaxHighlightingRulesDirectoryPath().str());
  ASSERT_EQ(tempDir / "data/fallback/", ResourcePaths::getFallbackDirectoryPath().str());
  ASSERT_EQ(tempDir / "data/fonts/", ResourcePaths::getFontsDirectoryPath().str());
  ASSERT_EQ(tempDir / "data/gui/", ResourcePaths::getGuiDirectoryPath().str());
  ASSERT_EQ(tempDir / "data/license/", ResourcePaths::getLicenseDirectoryPath().str());
  ASSERT_EQ(tempDir / "data/cxx/include/", ResourcePaths::getCxxCompilerHeaderDirectoryPath().str());
}