#include "IndexerPluginRegistry.h"

#include <algorithm>

#include <fmt/format.h>

#include "AppPath.h"
#include "ConfigManager.hpp"
#include "FileSystem.h"
#include "logging.h"
#include "TextAccess.h"
#include "utilityString.h"

IndexerPluginRegistry::Ptr IndexerPluginRegistry::s_instance;

IndexerPluginRegistry::Ptr IndexerPluginRegistry::getInstance() {
  if(!s_instance) {
    s_instance = std::shared_ptr<IndexerPluginRegistry>(new IndexerPluginRegistry());
  }
  return s_instance;
}

void IndexerPluginRegistry::destroyInstance() {
  s_instance.reset();
}

IndexerPluginRegistry::IndexerPluginRegistry() = default;

void IndexerPluginRegistry::discover() {
  discover(AppPath::getPluginsDirectoryPath());
}

void IndexerPluginRegistry::discover(const FilePath& pluginsDirectoryPath) {
  m_plugins.clear();

  LOG_DEBUG(L"Scanning indexer plugins directory: " + pluginsDirectoryPath.wstr());

  if(!pluginsDirectoryPath.exists()) {
    LOG_DEBUG(L"Indexer plugins directory does not exist, no plugins discovered: " + pluginsDirectoryPath.wstr());
    return;
  }

  for(const FilePath& pluginDirectoryPath : FileSystem::getDirectSubDirectories(pluginsDirectoryPath)) {
    const FilePath manifestFilePath = pluginDirectoryPath.getConcatenated(L"/manifest.xml");
    if(!manifestFilePath.exists()) {
      continue;
    }

    if(std::optional<Plugin> plugin = loadManifest(manifestFilePath); plugin.has_value()) {
      std::string sourceGroupTypesString;
      for(SourceGroupType type : plugin->sourceGroupTypes) {
        if(!sourceGroupTypesString.empty()) {
          sourceGroupTypesString += ", ";
        }
        sourceGroupTypesString += sourceGroupTypeToString(type);
      }

      LOG_DEBUG(fmt::format(
          "Discovered indexer plugin \"{}\" (id: {}, language: {}, commandType: {}, sourceGroupTypes: [{}], executable: {})",
          plugin->name,
          plugin->id,
          languageTypeToString(plugin->language),
          indexerCommandTypeToString(plugin->commandType),
          sourceGroupTypesString,
          plugin->indexerExecutablePath.str()));
      m_plugins.push_back(std::move(plugin.value()));
    } else {
      LOG_WARNING(L"Failed to load indexer plugin manifest: " + manifestFilePath.wstr());
    }
  }

  LOG_DEBUG(fmt::format("Indexer plugin discovery complete: {} plugin(s) found", m_plugins.size()));
}

bool IndexerPluginRegistry::supportsSourceGroupType(SourceGroupType type) const {
  return std::any_of(m_plugins.begin(), m_plugins.end(), [type](const Plugin& plugin) {
    return std::find(plugin.sourceGroupTypes.begin(), plugin.sourceGroupTypes.end(), type) != plugin.sourceGroupTypes.end();
  });
}

bool IndexerPluginRegistry::supportsLanguage(LanguageType language) const {
  return std::any_of(m_plugins.begin(), m_plugins.end(), [language](const Plugin& plugin) { return plugin.language == language; });
}

std::vector<SourceGroupType> IndexerPluginRegistry::availableSourceGroupTypes() const {
  std::vector<SourceGroupType> types;
  for(const Plugin& plugin : m_plugins) {
    for(SourceGroupType type : plugin.sourceGroupTypes) {
      if(std::find(types.begin(), types.end(), type) == types.end()) {
        types.push_back(type);
      }
    }
  }
  return types;
}

FilePath IndexerPluginRegistry::indexerExecutablePathFor(IndexerCommandType commandType) const {
  if(std::optional<Plugin> plugin = pluginFor(commandType); plugin.has_value()) {
    return plugin->indexerExecutablePath;
  }
  return FilePath();
}

std::optional<IndexerPluginRegistry::Plugin> IndexerPluginRegistry::pluginFor(IndexerCommandType commandType) const {
  for(const Plugin& plugin : m_plugins) {
    if(plugin.commandType == commandType) {
      return plugin;
    }
  }
  return std::nullopt;
}

const std::vector<IndexerPluginRegistry::Plugin>& IndexerPluginRegistry::getPlugins() const {
  return m_plugins;
}

std::optional<IndexerPluginRegistry::Plugin> IndexerPluginRegistry::loadManifest(const FilePath& manifestFilePath) {
  const ConfigManager::Ptr config = ConfigManager::createAndLoad(TextAccess::createFromFile(manifestFilePath));
  if(!config) {
    return std::nullopt;
  }

  Plugin plugin;
  plugin.id = config->getValueOrDefault<std::string>("id", "");
  plugin.name = config->getValueOrDefault<std::string>("name", "");
  plugin.language = stringToLanguageType(config->getValueOrDefault<std::string>("language", ""));
  plugin.commandType = stringToIndexerCommandType(config->getValueOrDefault<std::string>("commandType", ""));

  if(plugin.id.empty()) {
    return std::nullopt;
  }

  for(const std::string& typeString : config->getValuesOrDefaults<std::string>("source_group_types/source_group_type", {})) {
    const SourceGroupType type = stringToSourceGroupType(typeString);
    if(type != SOURCE_GROUP_UNKNOWN) {
      plugin.sourceGroupTypes.push_back(type);
    }
  }

  const std::string indexerExecutable = config->getValueOrDefault<std::string>("indexerExecutable", "");
  if(indexerExecutable.empty()) {
    return std::nullopt;
  }
  plugin.indexerExecutablePath = manifestFilePath.getParentDirectory().getConcatenated(utility::decodeFromUtf8(indexerExecutable));
  if(!plugin.indexerExecutablePath.exists()) {
    // A manifest without its binary is worse than no manifest: the project still looks
    // reindexable and the failure only surfaces once indexing has already started.
    LOG_WARNING(L"Ignoring indexer plugin \"" + utility::decodeFromUtf8(plugin.id) + L"\": executable is missing at \"" +
                plugin.indexerExecutablePath.wstr() + L"\".");
    return std::nullopt;
  }

  const std::string launcher = config->getValueOrDefault<std::string>("launcher", "");
  if(!launcher.empty()) {
    plugin.launcherPath = FilePath(utility::decodeFromUtf8(launcher));
    for(const std::string& arg : config->getValuesOrDefaults<std::string>("launcherArgs/arg", {})) {
      plugin.launcherArgs.push_back(utility::decodeFromUtf8(arg));
    }
  }

  return plugin;
}
