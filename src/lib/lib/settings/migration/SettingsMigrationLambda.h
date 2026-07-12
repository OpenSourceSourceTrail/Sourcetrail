#pragma once
// STL
#include <functional>
// internal
#include "SettingsMigration.h"

class SettingsMigrationLambda final : public SettingsMigration {
public:
  SettingsMigrationLambda(std::function<void(const SettingsMigration*, Settings*)> m_lambda);
  virtual ~SettingsMigrationLambda();
  virtual void apply(Settings* migratable) const;

private:
  std::function<void(const SettingsMigration*, Settings*)> m_lambda;
};