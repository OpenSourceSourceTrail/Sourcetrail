#pragma once

#include "project/SourceGroupFactoryModule.h"

class SourceGroupFactoryModuleJava : public SourceGroupFactoryModule {
public:
  [[nodiscard]] bool supports(SourceGroupType type) const override;
  [[nodiscard]] std::shared_ptr<SourceGroup> createSourceGroup(std::shared_ptr<SourceGroupSettings> settings) const override;
};
