#pragma once

#include "settings/source_group/component/SourceGroupSettingsWithSourceExtensions.h"

class SourceGroupSettingsWithSourceExtensionsJava : public SourceGroupSettingsWithSourceExtensions {
private:
  std::vector<std::wstring> getDefaultSourceExtensions() const override {
    return {L".java"};
  }
};
