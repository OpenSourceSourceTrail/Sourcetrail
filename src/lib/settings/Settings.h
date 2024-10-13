#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ConfigManager.hpp"
#include "FilePath.h"

class SettingsMigration;

/**
 * Base Setting
 */
class Settings {
public:
  Settings(const Settings& other);
  Settings& operator=(const Settings& other);
  Settings(Settings&& other) noexcept;
  Settings& operator=(Settings&& other) noexcept;

  virtual ~Settings();

  virtual bool load(const FilePath& filePath, bool readOnly = false);

  virtual bool loadFromString(const std::string& text, bool readOnly = false);

  virtual bool save();

  virtual bool save(const FilePath& filePath);

  virtual void clear();

  virtual const FilePath& getFilePath() const;

  size_t getVersion() const;

  void setVersion(size_t version);

protected:
  Settings();

  void setFilePath(const FilePath& filePath);

  template <typename T>
  T getValue(const std::string& key, T defaultValue) const;

  template <typename T>
  std::vector<T> getValues(const std::string& key, std::vector<T> defaultValues) const;

  std::vector<FilePath> getPathValues(const std::string& key) const;

  std::vector<std::filesystem::path> getPathValuesStl(const std::string& key) const noexcept;

  template <typename T>
  bool setValue(const std::string& key, T value);

  template <typename T>
  bool setValues(const std::string& key, std::vector<T> values);

  bool setPathValues(const std::string& key, const std::vector<FilePath>& paths);

  bool setPathValues(const std::string& key, const std::vector<std::filesystem::path>& paths) noexcept;

  bool isValueDefined(const std::string& key) const;

  void removeValues(const std::string& key);

  std::shared_ptr<ConfigManager> m_config;

private:
  FilePath m_filePath;
  bool m_readOnly = false;

  friend SettingsMigration;
};

template <typename T>
T Settings::getValue(const std::string& key, T defaultValue) const {
  if(m_config) {
    if(std::optional<T> result = m_config->getValue<T>(key); result.has_value()) {
      return result.value();
    }
  }
  return defaultValue;
}

template <typename T>
std::vector<T> Settings::getValues(const std::string& key, std::vector<T> defaultValues) const {
  if(m_config) {
    if(auto values = m_config->getValues<T>(key); values.has_value()) {
      return values.value();
    }
  }
  return defaultValues;
}

template <typename T>
bool Settings::setValue(const std::string& key, T value) {
  if(m_config) {
    m_config->setValue(key, value);
    return true;
  }
  return false;
}

template <typename T>
bool Settings::setValues(const std::string& key, std::vector<T> values) {
  if(m_config) {
    m_config->setValues(key, values);
    return true;
  }
  return false;
}