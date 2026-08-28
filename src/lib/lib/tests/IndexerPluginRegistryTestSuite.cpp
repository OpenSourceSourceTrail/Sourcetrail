#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "app/IndexerPluginRegistry.h"
#include "FilePath.h"

namespace {

class IndexerPluginRegistryFixture : public ::testing::Test {
protected:
  void SetUp() override {
    mPluginsDirectory = std::filesystem::temp_directory_path() / "IndexerPluginRegistryTestSuite";
    std::filesystem::remove_all(mPluginsDirectory);
    std::filesystem::create_directories(mPluginsDirectory);
  }

  void TearDown() override {
    std::filesystem::remove_all(mPluginsDirectory);
  }

  void writeManifest(const std::string& pluginId, const std::string& manifestContent) {
    const std::filesystem::path pluginDirectory = mPluginsDirectory / pluginId;
    std::filesystem::create_directories(pluginDirectory);

    std::ofstream stream(pluginDirectory / "manifest.xml");
    stream << manifestContent;
    stream.close();

    std::ofstream executable(pluginDirectory / "indexer");
    executable.close();
  }

  std::filesystem::path mPluginsDirectory;
};

}    // namespace

TEST(IndexerPluginRegistry, instance) {
  auto instance = IndexerPluginRegistry::getInstance();
  ASSERT_EQ(2, instance.use_count());
  IndexerPluginRegistry::destroyInstance();
  ASSERT_EQ(1, instance.use_count());
}

TEST_F(IndexerPluginRegistryFixture, discoversNothingWhenDirectoryMissing) {
  auto registry = IndexerPluginRegistry::getInstance();
  registry->discover(FilePath(mPluginsDirectory.string() + "/does_not_exist"));

  EXPECT_TRUE(registry->getPlugins().empty());
  EXPECT_FALSE(registry->supportsSourceGroupType(SOURCE_GROUP_CUSTOM_COMMAND));
  EXPECT_TRUE(registry->availableSourceGroupTypes().empty());
}

TEST_F(IndexerPluginRegistryFixture, parsesManifestAndAnswersCapabilityQueries) {
  writeManifest("custom",
                R"(<?xml version="1.0"?>
<config>
  <id>custom</id>
  <name>Custom Plugin</name>
  <language>Custom</language>
  <commandType>indexer_command_custom</commandType>
  <indexerExecutable>indexer</indexerExecutable>
  <source_group_types>
    <source_group_type>Custom Command Source Group</source_group_type>
  </source_group_types>
</config>
)");

  auto registry = IndexerPluginRegistry::getInstance();
  registry->discover(FilePath(mPluginsDirectory.string()));

  ASSERT_EQ(1, registry->getPlugins().size());
  const IndexerPluginRegistry::Plugin& plugin = registry->getPlugins().front();
  EXPECT_EQ("custom", plugin.id);
  EXPECT_EQ("Custom Plugin", plugin.name);
  EXPECT_EQ(LANGUAGE_CUSTOM, plugin.language);
  EXPECT_EQ(INDEXER_COMMAND_CUSTOM, plugin.commandType);
  EXPECT_TRUE(plugin.indexerExecutablePath.exists());

  EXPECT_TRUE(registry->supportsSourceGroupType(SOURCE_GROUP_CUSTOM_COMMAND));
  EXPECT_TRUE(registry->supportsLanguage(LANGUAGE_CUSTOM));
  EXPECT_FALSE(registry->supportsLanguage(LANGUAGE_UNKNOWN));

  const std::vector<SourceGroupType> available = registry->availableSourceGroupTypes();
  ASSERT_EQ(1, available.size());
  EXPECT_EQ(SOURCE_GROUP_CUSTOM_COMMAND, available.front());

  EXPECT_EQ(plugin.indexerExecutablePath, registry->indexerExecutablePathFor(INDEXER_COMMAND_CUSTOM));
}

TEST_F(IndexerPluginRegistryFixture, parsesJavaManifestAndAnswersCapabilityQueries) {
  writeManifest("java_stub",
                R"(<?xml version="1.0"?>
<config>
  <id>java_stub</id>
  <name>Java (stub)</name>
  <language>Java</language>
  <commandType>indexer_command_java</commandType>
  <indexerExecutable>indexer</indexerExecutable>
  <source_group_types>
    <source_group_type>Java Source Group</source_group_type>
  </source_group_types>
</config>
)");

  auto registry = IndexerPluginRegistry::getInstance();
  registry->discover(FilePath(mPluginsDirectory.string()));

  ASSERT_EQ(1, registry->getPlugins().size());
  const IndexerPluginRegistry::Plugin& plugin = registry->getPlugins().front();
  EXPECT_EQ(LANGUAGE_JAVA, plugin.language);
  EXPECT_EQ(INDEXER_COMMAND_JAVA, plugin.commandType);

  EXPECT_TRUE(registry->supportsSourceGroupType(SOURCE_GROUP_JAVA_EMPTY));
  EXPECT_TRUE(registry->supportsLanguage(LANGUAGE_JAVA));
  EXPECT_EQ(plugin.indexerExecutablePath, registry->indexerExecutablePathFor(INDEXER_COMMAND_JAVA));
}

TEST_F(IndexerPluginRegistryFixture, ignoresPluginDirectoryWithoutManifest) {
  std::filesystem::create_directories(mPluginsDirectory / "no_manifest");

  auto registry = IndexerPluginRegistry::getInstance();
  registry->discover(FilePath(mPluginsDirectory.string()));

  EXPECT_TRUE(registry->getPlugins().empty());
}

TEST_F(IndexerPluginRegistryFixture, ignoresManifestMissingRequiredFields) {
  writeManifest("broken",
                R"(<?xml version="1.0"?>
<config>
  <name>Broken Plugin</name>
</config>
)");

  auto registry = IndexerPluginRegistry::getInstance();
  registry->discover(FilePath(mPluginsDirectory.string()));

  EXPECT_TRUE(registry->getPlugins().empty());
}
