#include "project/logic/HeaderSearchDetection.h"

#include <cmath>

#include "FileManager.h"
#include "settings/IApplicationSettings.hpp"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPathsAndFlags.h"
#include "settings/source_group/component/SourceGroupSettingsWithExcludeFilters.h"
#include "settings/source_group/component/SourceGroupSettingsWithSourceExtensions.h"
#include "settings/source_group/component/SourceGroupSettingsWithSourcePaths.h"
#include "settings/source_group/SourceGroupSettings.h"
#include "utility.h"
#include "utility/IncludeProcessing.h"
#include "utilityFile.h"

namespace {

/** Walks the source tree the inputs describe. Reports indeterminate progress -- it cannot know the count. */
std::set<FilePath> gatherSourceFiles(const utility::HeaderDetectionInputs& inputs, const utility::DetectionProgress& progress) {
  progress(L"Gathering Source Files", std::nullopt);

  FileManager fileManager;
  fileManager.update(inputs.sourcePaths, inputs.excludeFilters, inputs.sourceExtensions);
  return fileManager.getAllSourceFilePaths();
}

/** Turns IncludeProcessing's 0..1 float into the message and percentage the caller reports. */
std::function<void(float)> fileProgress(size_t fileCount, const utility::DetectionProgress& progress) {
  return [fileCount, &progress](const float fraction) {
    progress(std::to_wstring(static_cast<int>(fraction * static_cast<float>(fileCount))) + L" Files",
             static_cast<size_t>(fraction * 100.0F));
  };
}

}    // namespace

namespace utility {

size_t detectionQuantileCount(size_t sourceFileCount) {
  return sourceFileCount == 0 ? 0 : static_cast<size_t>(std::log2(static_cast<double>(sourceFileCount)));
}

std::optional<HeaderDetectionInputs> collectDetectionInputs(const std::shared_ptr<SourceGroupSettings>& settings) {
  const auto extensionSettings = std::dynamic_pointer_cast<SourceGroupSettingsWithSourceExtensions>(settings);
  const auto pathSettings = std::dynamic_pointer_cast<SourceGroupSettingsWithSourcePaths>(settings);
  const auto excludeFilterSettings = std::dynamic_pointer_cast<SourceGroupSettingsWithExcludeFilters>(settings);
  if(!extensionSettings || !pathSettings || !excludeFilterSettings) {
    return std::nullopt;
  }

  HeaderDetectionInputs inputs;
  inputs.sourcePaths = pathSettings->getSourcePathsExpandedAndAbsolute();
  inputs.excludeFilters = excludeFilterSettings->getExcludeFiltersExpandedAndAbsolute();
  inputs.sourceExtensions = extensionSettings->getSourceExtensions();

  inputs.headerSearchPaths = utility::toFilePath(IApplicationSettings::getInstanceRaw()->getHeaderSearchPathsExpanded());
  if(const auto cxxSettings = std::dynamic_pointer_cast<SourceGroupSettingsWithCxxPathsAndFlags>(settings)) {
    utility::append(inputs.headerSearchPaths, cxxSettings->getHeaderSearchPathsExpandedAndAbsolute());
  }
  return inputs;
}

std::vector<IncludeDirective> findUnresolvedIncludes(const HeaderDetectionInputs& inputs, const DetectionProgress& progress) {
  const std::set<FilePath> sourceFilePaths = gatherSourceFiles(inputs, progress);

  return IncludeProcessing::getUnresolvedIncludeDirectives(sourceFilePaths,
                                                           utility::toSet(inputs.sourcePaths),
                                                           utility::toSet(inputs.headerSearchPaths),
                                                           detectionQuantileCount(sourceFilePaths.size()),
                                                           fileProgress(sourceFilePaths.size(), progress));
}

std::set<FilePath> detectHeaderSearchDirectories(const HeaderDetectionInputs& inputs,
                                                 const std::set<FilePath>& searchedPaths,
                                                 const DetectionProgress& progress) {
  const std::set<FilePath> sourceFilePaths = gatherSourceFiles(inputs, progress);

  return IncludeProcessing::getHeaderSearchDirectories(sourceFilePaths,
                                                       searchedPaths,
                                                       utility::toSet(inputs.headerSearchPaths),
                                                       detectionQuantileCount(sourceFilePaths.size()),
                                                       fileProgress(sourceFilePaths.size(), progress));
}

}    // namespace utility
