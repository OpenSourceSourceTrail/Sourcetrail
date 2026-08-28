#ifndef SOURCE_GROUP_SETTINGS_C_EMPTY_H
#define SOURCE_GROUP_SETTINGS_C_EMPTY_H

#include "settings/source_group/component/cxx/SourceGroupSettingsWithCStandard.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxCrossCompilationOptions.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPathsAndFlags.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPchOptions.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithSourceExtensionsC.h"
#include "settings/source_group/component/SourceGroupSettingsWithExcludeFilters.h"
#include "settings/source_group/component/SourceGroupSettingsWithSourcePaths.h"
#include "settings/source_group/SourceGroupSettingsWithComponents.h"

class SourceGroupSettingsCEmpty
    : public SourceGroupSettingsWithComponents<SourceGroupSettingsWithCStandard,
                                               SourceGroupSettingsWithCxxCrossCompilationOptions,
                                               SourceGroupSettingsWithCxxPathsAndFlags,
                                               SourceGroupSettingsWithCxxPchOptions,
                                               SourceGroupSettingsWithExcludeFilters,
                                               SourceGroupSettingsWithSourceExtensionsC,
                                               SourceGroupSettingsWithSourcePaths> {
public:
  SourceGroupSettingsCEmpty(const std::string& id, const ProjectSettings* projectSettings)
      : SourceGroupSettingsWithComponents(SOURCE_GROUP_C_EMPTY, id, projectSettings) {}

  std::shared_ptr<SourceGroupSettings> createCopy() const override {
    return std::make_shared<SourceGroupSettingsCEmpty>(*this);
  }
};

#endif    // SOURCE_GROUP_SETTINGS_C_EMPTY_H
