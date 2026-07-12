#include "SourceGroupJavaEmpty.h"

#include <utility>

#include "FileManager.h"
#include "IndexerCommandJava.h"
#include "MemoryIndexerCommandProvider.h"
#include "RefreshInfo.h"
#include "SourceGroupSettingsJavaEmpty.h"
#include "utility.h"

SourceGroupJavaEmpty::SourceGroupJavaEmpty(std::shared_ptr<SourceGroupSettingsJavaEmpty> settings)
    : m_settings(std::move(settings)) {}

std::set<FilePath> SourceGroupJavaEmpty::filterToContainedFilePaths(const std::set<FilePath>& filePaths) const {
  return SourceGroup::filterToContainedFilePaths(filePaths,
                                                 std::set<FilePath>(),
                                                 utility::toSet(m_settings->getSourcePathsExpandedAndAbsolute()),
                                                 m_settings->getExcludeFiltersExpandedAndAbsolute());
}

std::set<FilePath> SourceGroupJavaEmpty::getAllSourceFilePaths() const {
  FileManager fileManager;
  fileManager.update(m_settings->getSourcePathsExpandedAndAbsolute(),
                     m_settings->getExcludeFiltersExpandedAndAbsolute(),
                     m_settings->getSourceExtensions());
  return fileManager.getAllSourceFilePaths();
}

std::shared_ptr<IndexerCommandProvider> SourceGroupJavaEmpty::getIndexerCommandProvider(const RefreshInfo& info) const {
  const std::set<FilePath> classPaths = utility::toSet(m_settings->getClassPathsExpandedAndAbsolute());
  const std::wstring languageStandard = m_settings->getLanguageStandard();

  std::vector<std::shared_ptr<IndexerCommand>> commands;
  for(const FilePath& sourcePath : getAllSourceFilePaths()) {
    if(info.filesToIndex.find(sourcePath) != info.filesToIndex.end()) {
      commands.push_back(std::make_shared<IndexerCommandJava>(sourcePath, classPaths, languageStandard));
    }
  }

  return std::make_shared<MemoryIndexerCommandProvider>(commands);
}

std::vector<std::shared_ptr<IndexerCommand>> SourceGroupJavaEmpty::getIndexerCommands(const RefreshInfo& info) const {
  return getIndexerCommandProvider(info)->consumeAllCommands();
}

std::shared_ptr<SourceGroupSettings> SourceGroupJavaEmpty::getSourceGroupSettings() {
  return m_settings;
}

std::shared_ptr<const SourceGroupSettings> SourceGroupJavaEmpty::getSourceGroupSettings() const {
  return m_settings;
}
