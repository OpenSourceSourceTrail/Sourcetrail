#ifndef SOURCE_GROUP_SETTINGS_CPP_EMPTY_H
#define SOURCE_GROUP_SETTINGS_CPP_EMPTY_H

#include "settings/source_group/component/cxx/SourceGroupSettingsWithCppStandard.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxCrossCompilationOptions.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPathsAndFlags.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPchOptions.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithSourceExtensionsCpp.h"
#include "settings/source_group/component/SourceGroupSettingsWithExcludeFilters.h"
#include "settings/source_group/component/SourceGroupSettingsWithSourcePaths.h"
#include "settings/source_group/SourceGroupSettingsWithComponents.h"

class SourceGroupSettingsCppEmpty
    : public SourceGroupSettingsWithComponents<SourceGroupSettingsWithCppStandard,
                                               SourceGroupSettingsWithCxxCrossCompilationOptions,
                                               SourceGroupSettingsWithCxxPathsAndFlags,
                                               SourceGroupSettingsWithCxxPchOptions,
                                               SourceGroupSettingsWithExcludeFilters,
                                               SourceGroupSettingsWithSourceExtensionsCpp,
                                               SourceGroupSettingsWithSourcePaths> {
public:
  SourceGroupSettingsCppEmpty(const std::string& id, const ProjectSettings* projectSettings)
      : SourceGroupSettingsWithComponents(SOURCE_GROUP_CPP_EMPTY, id, projectSettings) {}

  std::shared_ptr<SourceGroupSettings> createCopy() const override {
    return std::make_shared<SourceGroupSettingsCppEmpty>(*this);
  }
};

#endif    // SOURCE_GROUP_SETTINGS_CPP_EMPTY_H
