#pragma once
// STL
#include <memory>
// internal
#include "SourceGroupType.h"

class SourceGroup;
class SourceGroupSettings;

class SourceGroupFactoryModule {
public:
  virtual ~SourceGroupFactoryModule();
  [[nodiscard]] virtual bool supports(SourceGroupType type) const = 0;
  [[nodiscard]] virtual std::shared_ptr<SourceGroup> createSourceGroup(std::shared_ptr<SourceGroupSettings> settings) const = 0;
};