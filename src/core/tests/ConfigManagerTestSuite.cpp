#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ConfigManager.hpp"
#include "TextAccess.h"

namespace {
constexpr std::string_view kTestingXml =
    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
    "<config>\n"
    "	<path>\n"
    "		<to>\n"
    "			<bool_that_is_false>0</bool_that_is_false>\n"
    "			<bool_that_is_true>1</bool_that_is_true>\n"
    "			<single_value>42</single_value>\n"
    "		</to>\n"
    "	</path>\n"
    "	<paths>\n"
    "		<nopath>4</nopath>\n"
    "		<path>2</path>\n"
    "		<path>5</path>\n"
    "		<path>8</path>\n"
    "	</paths>\n"
    "</config>\n";
constexpr std::string_view kRealProjectXml =
    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
    "<config>\n"
    "    <source_groups>\n"
    "        <source_group_3bd75114-1d08-41b1-8985-1bbe691afb06>\n"
    "            <build_file_path>\n"
    "                <compilation_db_path>../build/Coverage/GNU/compile_commands.json</compilation_db_path>\n"
    "            </build_file_path>\n"
    "        </source_group_3bd75114-1d08-41b1-8985-1bbe691afb06>\n"
    "    </source_groups>\n"
    "</config>\n";
std::shared_ptr<TextAccess> getConfigTextAccess(std::string_view text = kTestingXml) {
  return TextAccess::createFromString(text.data());
}

struct ConfigManagerFix : testing::Test {
  void SetUp() override {
    configManager = ConfigManager::createAndLoad(getConfigTextAccess());
  }

  ConfigManager::Ptr configManager;
};

TEST_F(ConfigManagerFix, DISABLED_loadIsCorrect) {
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: get value with valid path
  const auto result = configManager->toString();
  // Then: a valid result
  ASSERT_THAT(result, testing::Not(testing::IsEmpty()));
  EXPECT_THAT(result, testing::StrEq(kTestingXml));
}

TEST_F(ConfigManagerFix, returnsTrueWhenKeyIsFound) {
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: get value with valid path
  const auto result = configManager->getValue<float>("path/to/single_value");
  // Then: a valid result
  ASSERT_TRUE(result);
  EXPECT_FLOAT_EQ(42, result.value());
}

TEST_F(ConfigManagerFix, returnsFalseWhenKeyIsNotFound) {
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: get value with invalid path
  const auto result = configManager->getValue<float>("path/to/nowhere");
  // Then: an invalid result
  EXPECT_FALSE(result);
}

TEST_F(ConfigManagerFix, returnsCorrectIntForKey) {
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: get value with a valid path
  const auto result = configManager->getValue<int>("path/to/single_value");
  // Then: a valid result
  ASSERT_TRUE(result);
  EXPECT_EQ(42, result.value());
}

TEST_F(ConfigManagerFix, returnsCorrectBoolForKeyIfValueIsTrue) {
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: get value with a valid path
  const auto result = configManager->getValue<bool>("path/to/bool_that_is_true");
  // Then: a valid result
  ASSERT_TRUE(result);
  EXPECT_TRUE(result.value());
}

TEST_F(ConfigManagerFix, returnsCorrectBoolForKeyIfValueIsFalse) {
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: get value with a valid path
  const auto result = configManager->getValue<bool>("path/to/bool_that_is_false");
  // Then: a valid result
  ASSERT_TRUE(result);
  EXPECT_FALSE(result.value());
}

TEST_F(ConfigManagerFix, addsNewKeyWhenNotEmpty) {
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: set value
  configManager->setValue("path/to/true_bool", true);
  // And: get the same value
  const auto result = configManager->getValue<bool>("path/to/true_bool");
  // Then: a valid result
  ASSERT_TRUE(result);
  EXPECT_TRUE(result.value());
}

TEST_F(ConfigManagerFix, returnsCorrectListForKey) {
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: get value
  auto result = configManager->getValues<int>("paths/path");
  // Then: a valid result
  ASSERT_TRUE(result.has_value());

  const auto& values = result.value();
  ASSERT_EQ(3, values.size());
  EXPECT_EQ(2, values[0]);
  EXPECT_EQ(5, values[1]);
  EXPECT_EQ(8, values[2]);
}

TEST_F(ConfigManagerFix, saveAndLoadConfigurationAndCompare) {
  constexpr std::string_view path = "data/ConfigManagerTestSuite/temp.xml";
  // Given: configManager is created and loaded successfully
  ASSERT_TRUE(configManager);
  // When: get value
  ASSERT_TRUE(configManager->save(path));
  // And: Load the config successfully
  const auto config2 = ConfigManager::createAndLoad(TextAccess::createFromFile(FilePath {path.data()}));
  // Then: the config us created
  ASSERT_TRUE(config2);
  EXPECT_THAT(configManager->toString(), testing::StrEq(config2->toString()));
}

TEST(ConfigManager, loadsSpecialCharacter) {
  // Given: configManager is created and loaded successfully
  auto config = ConfigManager::createAndLoad(TextAccess::createFromFile(FilePath(L"data/ConfigManagerTestSuite/test_data.xml")));
  ASSERT_TRUE(config);
  // When: get value
  const auto result = config->getValue<std::wstring>("path/to/special_character");
  // Then: the config us created
  ASSERT_TRUE(result);
  const std::wstring& loadedSpecialCharacter = result.value();
  ASSERT_EQ(1, loadedSpecialCharacter.size());
  EXPECT_TRUE(loadedSpecialCharacter[0] == wchar_t(252));
}

TEST(ConfigManager, saveAndLoadSpecialCharacterAndCompare) {
  const FilePath path(L"data/ConfigManagerTestSuite/temp.xml");
  const std::wstring specialCharacter {wchar_t(252)};

  // Given: configManager is created successfully
  auto config = ConfigManager::createEmpty();
  ASSERT_TRUE(config);
  // And: set a spec char
  config->setValue("path/to/special_character", specialCharacter);
  // And: save it to config
  ASSERT_TRUE(config->save(path.str()));
  // And: Loaded successfully
  auto config2 = ConfigManager::createAndLoad(TextAccess::createFromFile(path));
  ASSERT_TRUE(config2);
  // Then: Both config are the same
  EXPECT_THAT(config->toString(), testing::StrEq(config2->toString()));
}

TEST(ConfigManager, addsNewKeyWhenEmpty) {
  // Given: configManager is created successfully
  auto config = ConfigManager::createEmpty();

  // And: save it to config
  config->setValue("path/to/true_bool", true);

  // When: get value
  const auto result = config->getValue<bool>("path/to/true_bool");

  // Then: The value exists and true
  ASSERT_TRUE(result);
  EXPECT_TRUE(result.value());
}

TEST(ConfigManager, loadConfig) {
  // Given: configManager is created and loaded successfully
  auto config = ConfigManager::createAndLoad(getConfigTextAccess(kRealProjectXml));
  ASSERT_TRUE(config);
  // When:
  const auto string = config->toString();
  // Then:
  ASSERT_THAT(string, testing::Not(testing::IsEmpty()));
}
}    // namespace
