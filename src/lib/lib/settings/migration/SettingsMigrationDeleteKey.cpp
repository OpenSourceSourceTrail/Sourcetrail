#include "SettingsMigrationDeleteKey.h"

SettingsMigrationDeleteKey::SettingsMigrationDeleteKey(std::string key) : m_key(std::move(key)) {}

SettingsMigrationDeleteKey::~SettingsMigrationDeleteKey() = default;

void SettingsMigrationDeleteKey::apply(Settings* migratable) const {
  removeValuesInSettings(migratable, m_key);
}