#pragma once

#include "SourceGroupSettingsWithComponents.h"
#include "SourceGroupSettingsWithExcludeFilters.h"
#include "SourceGroupSettingsWithJavaClassPath.h"
#include "SourceGroupSettingsWithSourceExtensionsJava.h"
#include "SourceGroupSettingsWithSourcePaths.h"

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
