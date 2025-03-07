#include "Settings.h"

#include <range/v3/to_container.hpp>
#include <range/v3/view/transform.hpp>

#include "logging.h"
#include "TextAccess.h"

Settings::Settings() {
  clear();    // NOLINT(clang-analyzer-optin.cplusplus.VirtualCall):  TODO fix the following
}

Settings::Settings(const Settings& other) : m_config(other.m_config->createCopy()), m_filePath(other.m_filePath) {}

Settings& Settings::operator=(const Settings& other) {
  if(&other != this) {
    m_filePath = other.m_filePath;
    m_config = other.m_config->createCopy();
  }

  return *this;
}

Settings::Settings(Settings&& other) noexcept = default;

Settings& Settings::operator=(Settings&& other) noexcept = default;

Settings::~Settings() = default;

bool Settings::load(const FilePath& filePath, bool readOnly) {
  m_readOnly = readOnly;

  if(filePath.exists()) {
    m_config = ConfigManager::createAndLoad(TextAccess::createFromFile(filePath));
    m_filePath = filePath;
    return true;
  }

  clear();
  m_filePath = filePath;
  LOG_WARNING(L"File for Settings not found: " + filePath.wstr());
  return false;
}

bool Settings::loadFromString(const std::string& text, bool readOnly) {
  m_readOnly = readOnly;

  m_config = ConfigManager::createAndLoad(TextAccess::createFromString(text));
  m_filePath = FilePath();
  return true;
}

bool Settings::save() {
  if(m_readOnly) {
    return false;
  }

  bool success = false;
  if(m_config && !m_filePath.empty()) {
    success = m_config->save(m_filePath.str());
  }

  if(!success) {
    LOG_WARNING(L"Settings were not saved: " + m_filePath.wstr());
  }

  return success;
}

bool Settings::save(const FilePath& filePath) {
  setFilePath(filePath);

  return save();
}

void Settings::clear() {
  m_config = ConfigManager::createEmpty();
  m_filePath = FilePath();
}

const FilePath& Settings::getFilePath() const {
  return m_filePath;
}

size_t Settings::getVersion() const {
  return static_cast<size_t>(getValue<int>("version", 0));
}

void Settings::setVersion(size_t version) {
  setValue<int>("version", static_cast<int>(version));
}

void Settings::setFilePath(const FilePath& filePath) {
  m_filePath = filePath;
}

std::vector<FilePath> Settings::getPathValues(const std::string& key) const {
  auto values = getValues<std::wstring>(key, {});
  return values | ranges::cpp20::views::transform([](const auto& value) { return FilePath{value}; }) | ranges::to<std::vector>();
}

std::vector<std::filesystem::path> Settings::getPathValuesStl(const std::string& key) const noexcept {
  auto values = getValues<std::wstring>(key, {});
  return values | ranges::cpp20::views::transform([](const auto& value) -> std::filesystem::path { return value; }) |
      ranges::to<std::vector>();
}

bool Settings::setPathValues(const std::string& key, const std::vector<FilePath>& paths) {
  const std::vector<std::wstring> values = paths |
      ranges::cpp20::views::transform([](const auto& value) { return value.wstr(); }) | ranges::to<std::vector>();
  return setValues(key, values);
}

bool Settings::setPathValues(const std::string& key, const std::vector<std::filesystem::path>& paths) noexcept {
  const std::vector<std::wstring> values = paths |
      ranges::cpp20::views::transform([](const auto& value) { return value.wstring(); }) | ranges::to<std::vector>();
  return setValues(key, values);
}

bool Settings::isValueDefined(const std::string& key) const {
  return m_config->isValueDefined(key);
}

void Settings::removeValues(const std::string& key) {
  m_config->removeValues(key);
}