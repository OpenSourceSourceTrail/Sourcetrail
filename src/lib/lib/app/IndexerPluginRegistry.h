#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "FilePath.h"
#include "indexing/domain/IndexerCommandType.h"
#include "settings/LanguageType.h"
#include "settings/source_group/SourceGroupType.h"

class IndexerPluginRegistry final {
public:
  struct Plugin {
    std::string id;
    std::string name;
    LanguageType language = LANGUAGE_UNKNOWN;
    std::vector<SourceGroupType> sourceGroupTypes;
    IndexerCommandType commandType = INDEXER_COMMAND_UNKNOWN;
    FilePath indexerExecutablePath;
    FilePath launcherPath;
    std::vector<std::wstring> launcherArgs;
  };

  using Ptr = std::shared_ptr<IndexerPluginRegistry>;
  static Ptr getInstance();
  static void destroyInstance();

  void discover();
  void discover(const FilePath& pluginsDirectoryPath);

  [[nodiscard]] bool supportsSourceGroupType(SourceGroupType type) const;
  [[nodiscard]] bool supportsLanguage(LanguageType language) const;

  [[nodiscard]] std::vector<SourceGroupType> availableSourceGroupTypes() const;

  [[nodiscard]] FilePath indexerExecutablePathFor(IndexerCommandType commandType) const;

  [[nodiscard]] std::optional<Plugin> pluginFor(IndexerCommandType commandType) const;

  [[nodiscard]] const std::vector<Plugin>& getPlugins() const;

private:
  static Ptr s_instance;
  static std::once_flag s_once;
  IndexerPluginRegistry();

  static std::optional<Plugin> loadManifest(const FilePath& manifestFilePath);

  std::vector<Plugin> m_plugins;
};
