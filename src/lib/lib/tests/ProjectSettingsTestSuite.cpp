#include <gtest/gtest.h>

#include "ProjectSettings.h"
#include "SourceGroupSettings.h"
#include "SourceGroupSettingsWithSourcePaths.h"

constexpr auto SettingsPath = L"data/SettingsTestSuite/settings.xml";

void PrintTo(LanguageType type, std::ostream* ostream) {
  switch(type) {
#if BUILD_CXX_LANGUAGE_PACKAGE
  case LANGUAGE_CPP:
    *ostream << "LANGUAGE_CPP";
    break;
  case LANGUAGE_C:
    *ostream << "LANGUAGE_C";
    break;
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  case LANGUAGE_CUSTOM:
    *ostream << "LANGUAGE_CUSTOM";
    break;
  case LANGUAGE_UNKNOWN:
    *ostream << "LANGUAGE_UNKNOWN";
    break;
  }
}

TEST(ProjectSettings, getLanguageOfProject) {
  EXPECT_EQ(LANGUAGE_CUSTOM, ProjectSettings::getLanguageOfProject(FilePath(SettingsPath)));
}

TEST(ProjectSettings, loadProjectSettingsFromFile) {
  ProjectSettings settings;
  EXPECT_TRUE(settings.load(FilePath(SettingsPath)));
}

TEST(ProjectSettings, loadSourcePathFromFile) {
  ProjectSettings projectSettings;
  projectSettings.load(FilePath(SettingsPath));
  const auto pSourceGroupSettings = std::dynamic_pointer_cast<SourceGroupSettingsWithSourcePaths>(
      projectSettings.getAllSourceGroupSettings().front());
  const auto paths = pSourceGroupSettings->getSourcePaths();

  EXPECT_EQ(2, paths.size());
  EXPECT_EQ(paths[0].wstr(), L"src");
  EXPECT_EQ(paths[1].wstr(), L"test");
}
