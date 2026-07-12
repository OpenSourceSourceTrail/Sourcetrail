#pragma once
// STL
#include <string>
// internal
#include "SettingsMigration.h"

class SettingsMigrationDeleteKey : public SettingsMigration {
public:
  explicit SettingsMigrationDeleteKey(std::string key);
  /**
   * @name Disable copy and move operators
   * @{ */
  SettingsMigrationDeleteKey(const SettingsMigrationDeleteKey&) = delete;
  SettingsMigrationDeleteKey(SettingsMigrationDeleteKey&&) = delete;
  SettingsMigrationDeleteKey& operator=(const SettingsMigrationDeleteKey&) = delete;
  SettingsMigrationDeleteKey& operator=(SettingsMigrationDeleteKey&&) = delete;
  /**  @} */
  ~SettingsMigrationDeleteKey() override;
  void apply(Settings* migratable) const override;

private:
  const std::string m_key;
};