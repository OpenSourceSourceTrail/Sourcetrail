#pragma once

#include "settings/source_group/component/java/SourceGroupSettingsWithJavaClassPath.h"
#include "settings/source_group/component/SourceGroupSettingsWithExcludeFilters.h"
#include "settings/source_group/component/SourceGroupSettingsWithSourceExtensionsJava.h"
#include "settings/source_group/component/SourceGroupSettingsWithSourcePaths.h"
#include "settings/source_group/SourceGroupSettingsWithComponents.h"

class SourceGroupSettingsJavaEmpty
    : public SourceGroupSettingsWithComponents<SourceGroupSettingsWithExcludeFilters,
                                               SourceGroupSettingsWithJavaClassPath,
                                               SourceGroupSettingsWithSourceExtensionsJava,
                                               SourceGroupSettingsWithSourcePaths> {
public:
  SourceGroupSettingsJavaEmpty(const std::string& id, const ProjectSettings* projectSettings)
      : SourceGroupSettingsWithComponents(SOURCE_GROUP_JAVA_EMPTY, id, projectSettings) {}

  std::shared_ptr<SourceGroupSettings> createCopy() const override {
    return std::make_shared<SourceGroupSettingsJavaEmpty>(*this);
  }
};
