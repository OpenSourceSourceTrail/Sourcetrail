#ifndef SOURCE_GROUP_SETTINGS_CXX_CDB_H
#define SOURCE_GROUP_SETTINGS_CXX_CDB_H

#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxCdbPath.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPathsAndFlags.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPchOptions.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithIndexedHeaderPaths.h"
#include "settings/source_group/component/SourceGroupSettingsWithExcludeFilters.h"
#include "settings/source_group/SourceGroupSettingsWithComponents.h"

class SourceGroupSettingsCxxCdb
    : public SourceGroupSettingsWithComponents<SourceGroupSettingsWithCxxCdbPath,
                                               SourceGroupSettingsWithCxxPathsAndFlags,
                                               SourceGroupSettingsWithCxxPchOptions,
                                               SourceGroupSettingsWithExcludeFilters,
                                               SourceGroupSettingsWithIndexedHeaderPaths> {
public:
  SourceGroupSettingsCxxCdb(const std::string& id, const ProjectSettings* projectSettings)
      : SourceGroupSettingsWithComponents(SOURCE_GROUP_CXX_CDB, id, projectSettings) {}

  std::shared_ptr<SourceGroupSettings> createCopy() const override {
    return std::make_shared<SourceGroupSettingsCxxCdb>(*this);
  }
};

#endif    // SOURCE_GROUP_SETTINGS_CXX_CDB_H
