#include "SourceGroupCxxEmpty.h"

#include <utility>

#include "CxxIndexerCommandProvider.h"
#include "FileManager.h"
#include "IApplicationSettings.hpp"
#include "IndexerCommandCxx.h"
#include "logging.h"
#include "RefreshInfo.h"
#include "SourceGroupSettingsCEmpty.h"
#include "SourceGroupSettingsCppEmpty.h"
#include "SourceGroupSettingsWithCppStandard.h"
#include "SourceGroupSettingsWithCStandard.h"
#include "SourceGroupSettingsWithCxxPathsAndFlags.h"
#include "TaskLambda.h"
#include "utility.h"
#include "utilitySourceGroupCxx.h"

SourceGroupCxxEmpty::SourceGroupCxxEmpty(std::shared_ptr<SourceGroupSettings> settings) : mSettings(std::move(settings)) {}

std::set<FilePath> SourceGroupCxxEmpty::filterToContainedFilePaths(const std::set<FilePath>& filePaths) const {
  std::vector<FilePath> indexedPaths;
  std::vector<FilePathFilter> excludeFilters;
  if(const std::shared_ptr<SourceGroupSettingsCEmpty> settings = std::dynamic_pointer_cast<SourceGroupSettingsCEmpty>(mSettings)) {
    indexedPaths = settings->getSourcePathsExpandedAndAbsolute();
    excludeFilters = settings->getExcludeFiltersExpandedAndAbsolute();
  } else if(const std::shared_ptr<SourceGroupSettingsCppEmpty> settingsCppEmpty =
                std::dynamic_pointer_cast<SourceGroupSettingsCppEmpty>(mSettings)) {
    indexedPaths = settingsCppEmpty->getSourcePathsExpandedAndAbsolute();
    excludeFilters = settingsCppEmpty->getExcludeFiltersExpandedAndAbsolute();
  }

  return SourceGroup::filterToContainedFilePaths(filePaths, std::set<FilePath>(), utility::toSet(indexedPaths), excludeFilters);
}

std::set<FilePath> SourceGroupCxxEmpty::getAllSourceFilePaths() const {
  FileManager fileManager;
  if(const std::shared_ptr<SourceGroupSettingsCEmpty> settings = std::dynamic_pointer_cast<SourceGroupSettingsCEmpty>(mSettings)) {
    fileManager.update(settings->getSourcePathsExpandedAndAbsolute(),
                       settings->getExcludeFiltersExpandedAndAbsolute(),
                       settings->getSourceExtensions());
  } else if(const std::shared_ptr<SourceGroupSettingsCppEmpty> settingsCppEmpty =
                std::dynamic_pointer_cast<SourceGroupSettingsCppEmpty>(mSettings)) {
    fileManager.update(settingsCppEmpty->getSourcePathsExpandedAndAbsolute(),
                       settingsCppEmpty->getExcludeFiltersExpandedAndAbsolute(),
                       settingsCppEmpty->getSourceExtensions());
  }

  return fileManager.getAllSourceFilePaths();
}

std::shared_ptr<IndexerCommandProvider> SourceGroupCxxEmpty::getIndexerCommandProvider(const RefreshInfo& info) const {
  std::set<FilePath> indexedPaths;
  std::set<FilePathFilter> excludeFilters;
  if(const std::shared_ptr<SourceGroupSettingsCEmpty> settings = std::dynamic_pointer_cast<SourceGroupSettingsCEmpty>(mSettings)) {
    indexedPaths = utility::toSet(settings->getSourcePathsExpandedAndAbsolute());
    excludeFilters = utility::toSet(settings->getExcludeFiltersExpandedAndAbsolute());
  } else if(const std::shared_ptr<SourceGroupSettingsCppEmpty> settingsCppEmpty =
                std::dynamic_pointer_cast<SourceGroupSettingsCppEmpty>(mSettings)) {
    indexedPaths = utility::toSet(settingsCppEmpty->getSourcePathsExpandedAndAbsolute());
    excludeFilters = utility::toSet(settingsCppEmpty->getExcludeFiltersExpandedAndAbsolute());
  }

  std::vector<std::wstring> compilerFlags = getBaseCompilerFlags();
  utility::append(compilerFlags, dynamic_cast<const SourceGroupSettingsWithCxxPathsAndFlags*>(mSettings.get())->getCompilerFlags());
  utility::append(
      compilerFlags, utility::getIncludePchFlags(dynamic_cast<const SourceGroupSettingsWithCxxPchOptions*>(mSettings.get())));

  std::shared_ptr<CxxIndexerCommandProvider> provider = std::make_shared<CxxIndexerCommandProvider>();
  for(const FilePath& sourcePath : getAllSourceFilePaths()) {
    if(info.filesToIndex.find(sourcePath) != info.filesToIndex.end()) {
      provider->addCommand(std::make_shared<IndexerCommandCxx>(sourcePath,
                                                               indexedPaths,
                                                               excludeFilters,
                                                               std::set<FilePathFilter>(),
                                                               mSettings->getProjectDirectoryPath(),
                                                               utility::concat(compilerFlags, sourcePath.wstr())));
    }
  }

  return provider;
}

std::vector<std::shared_ptr<IndexerCommand>> SourceGroupCxxEmpty::getIndexerCommands(const RefreshInfo& info) const {
  return getIndexerCommandProvider(info)->consumeAllCommands();
}

std::shared_ptr<Task> SourceGroupCxxEmpty::getPreIndexTask(std::shared_ptr<StorageProvider> storageProvider,
                                                           std::shared_ptr<DialogView> dialogView) const {
  const auto* pchSettings = dynamic_cast<const SourceGroupSettingsWithCxxPchOptions*>(mSettings.get());
  if(nullptr == pchSettings || pchSettings->getPchInputFilePath().empty()) {
    return std::make_shared<TaskLambda>([]() {});
  }

  std::vector<std::wstring> compilerFlags = getBaseCompilerFlags();

  if(const std::shared_ptr<SourceGroupSettingsWithCxxPchOptions> cxxPchOptions =
         std::dynamic_pointer_cast<SourceGroupSettingsWithCxxPchOptions>(mSettings)) {
    if(cxxPchOptions->getUseCompilerFlags()) {
      utility::append(
          compilerFlags, dynamic_cast<const SourceGroupSettingsWithCxxPathsAndFlags*>(mSettings.get())->getCompilerFlags());
    }

    utility::append(compilerFlags, cxxPchOptions->getPchFlags());
  }

  return utility::createBuildPchTask(
      dynamic_cast<const SourceGroupSettingsWithCxxPchOptions*>(mSettings.get()), compilerFlags, storageProvider, dialogView);
}

std::shared_ptr<SourceGroupSettings> SourceGroupCxxEmpty::getSourceGroupSettings() {
  return mSettings;
}

std::shared_ptr<const SourceGroupSettings> SourceGroupCxxEmpty::getSourceGroupSettings() const {
  return mSettings;
}

std::vector<std::wstring> SourceGroupCxxEmpty::getBaseCompilerFlags() const {
  std::vector<std::wstring> compilerFlags;

  auto* appSettings = IApplicationSettings::getInstanceRaw();
  std::set<FilePath> indexedPaths;
  std::wstring targetFlag;
  std::wstring languageStandard = SourceGroupSettingsWithCppStandard::getDefaultCppStandardStatic();

  if(const std::shared_ptr<SourceGroupSettingsCEmpty> settings = std::dynamic_pointer_cast<SourceGroupSettingsCEmpty>(mSettings)) {
    indexedPaths = utility::toSet(settings->getSourcePathsExpandedAndAbsolute());
    targetFlag = settings->getTargetFlag();
    languageStandard = settings->getCStandard();
  } else if(const std::shared_ptr<SourceGroupSettingsCppEmpty> settingsCppEmpty =
                std::dynamic_pointer_cast<SourceGroupSettingsCppEmpty>(mSettings)) {
    indexedPaths = utility::toSet(settingsCppEmpty->getSourcePathsExpandedAndAbsolute());
    targetFlag = settingsCppEmpty->getTargetFlag();
    languageStandard = settingsCppEmpty->getCppStandard();
  } else {
    LOG_ERROR(L"Source group doesn't specify language standard. Falling back to \"" + languageStandard + L"\".");
  }

  if(!targetFlag.empty()) {
    compilerFlags.push_back(targetFlag);
  }

  compilerFlags.push_back(IndexerCommandCxx::getCompilerFlagLanguageStandard(languageStandard));

  if(std::dynamic_pointer_cast<SourceGroupSettingsCEmpty>(mSettings)) {
    compilerFlags.emplace_back(L"-x");
    compilerFlags.emplace_back(L"c");
  } else if(std::dynamic_pointer_cast<SourceGroupSettingsCppEmpty>(mSettings)) {
    compilerFlags.emplace_back(L"-x");
    compilerFlags.emplace_back(L"c++");
  }

  const auto* settingsCxx = dynamic_cast<const SourceGroupSettingsWithCxxPathsAndFlags*>(mSettings.get());

  if(nullptr == settingsCxx) {
    LOG_ERROR(L"Source group doesn't specify cxx headers and flags.");
    return compilerFlags;
  }

  {
    // Add the source paths as HeaderSearchPaths as well, so clang will also look here when
    // searching include files.
    std::vector<FilePath> indexedDirectoryPaths;
    for(const FilePath& sourcePath : indexedPaths) {
      if(sourcePath.isDirectory()) {
        indexedDirectoryPaths.push_back(sourcePath);
      }
    }

    utility::append(compilerFlags,
                    IndexerCommandCxx::getCompilerFlagsForSystemHeaderSearchPaths(
                        utility::concat(indexedDirectoryPaths,
                                        utility::concat(settingsCxx->getHeaderSearchPathsExpandedAndAbsolute(),
                                                        utility::toFilePath(appSettings->getHeaderSearchPathsExpanded())))));
  }
  {
    utility::append(compilerFlags,
                    IndexerCommandCxx::getCompilerFlagsForFrameworkSearchPaths(
                        utility::concat(settingsCxx->getFrameworkSearchPathsExpandedAndAbsolute(),
                                        utility::toFilePath(appSettings->getFrameworkSearchPathsExpanded()))));
  }

  return compilerFlags;
}
