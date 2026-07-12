#include <filesystem>

#include <gtest/gtest.h>

#include "ProjectSettings.h"
#include "SourceGroupSettings.h"
#include "SourceGroupSettingsJavaEmpty.h"
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
  case LANGUAGE_JAVA:
    *ostream << "LANGUAGE_JAVA";
    break;
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

TEST(ProjectSettings, javaSourceGroupRoundTripsThroughSaveAndLoad) {
  const FilePath projectFilePath((std::filesystem::temp_directory_path() / "ProjectSettingsTestSuite_java.srctrlprj").wstring());

  {
    ProjectSettings settings;
    auto javaSettings = std::make_shared<SourceGroupSettingsJavaEmpty>("java_group_id", &settings);
    javaSettings->setSourcePaths({FilePath(L"src")});
    javaSettings->setClassPaths({FilePath(L"lib/dependency.jar")});
    settings.setAllSourceGroupSettings({javaSettings});
    ASSERT_TRUE(settings.save(projectFilePath));
  }

  ProjectSettings loadedSettings;
  ASSERT_TRUE(loadedSettings.load(projectFilePath));

  const auto allSettings = loadedSettings.getAllSourceGroupSettings();
  ASSERT_EQ(1, allSettings.size());
  EXPECT_EQ(SOURCE_GROUP_JAVA_EMPTY, allSettings.front()->getType());

  const auto javaSettings = std::dynamic_pointer_cast<SourceGroupSettingsJavaEmpty>(allSettings.front());
  ASSERT_TRUE(javaSettings);
  ASSERT_EQ(1, javaSettings->getSourcePaths().size());
  EXPECT_EQ(L"src", javaSettings->getSourcePaths().front().wstr());
  ASSERT_EQ(1, javaSettings->getClassPaths().size());
  EXPECT_EQ(L"lib/dependency.jar", javaSettings->getClassPaths().front().wstr());

  std::filesystem::remove(projectFilePath.wstr());
}
