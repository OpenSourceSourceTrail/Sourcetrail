#include "settings/source_group/component/java/SourceGroupSettingsWithJavaClassPath.h"

#include "settings/ProjectSettings.h"
#include "utility.h"

std::vector<FilePath> SourceGroupSettingsWithJavaClassPath::getClassPaths() const {
  return m_classPaths;
}

std::vector<FilePath> SourceGroupSettingsWithJavaClassPath::getClassPathsExpandedAndAbsolute() const {
  return getProjectSettings()->makePathsExpandedAndAbsolute(getClassPaths());
}

void SourceGroupSettingsWithJavaClassPath::setClassPaths(const std::vector<FilePath>& classPaths) {
  m_classPaths = classPaths;
}

std::wstring SourceGroupSettingsWithJavaClassPath::getLanguageStandard() const {
  return m_languageStandard;
}

void SourceGroupSettingsWithJavaClassPath::setLanguageStandard(const std::wstring& languageStandard) {
  m_languageStandard = languageStandard;
}

bool SourceGroupSettingsWithJavaClassPath::equals(const SourceGroupSettingsBase* other) const {
  const SourceGroupSettingsWithJavaClassPath* otherPtr = dynamic_cast<const SourceGroupSettingsWithJavaClassPath*>(other);

  return (otherPtr && utility::isPermutation(m_classPaths, otherPtr->m_classPaths) &&
          m_languageStandard == otherPtr->m_languageStandard);
}

void SourceGroupSettingsWithJavaClassPath::load(const ConfigManager* config, const std::string& key) {
  setClassPaths(config->getValuesOrDefaults(key + "/class_paths/class_path", std::vector<FilePath>()));
  setLanguageStandard(config->getValueOrDefault(key + "/language_standard", std::wstring(L"17")));
}

void SourceGroupSettingsWithJavaClassPath::save(ConfigManager* config, const std::string& key) {
  config->setValues(key + "/class_paths/class_path", getClassPaths());
  config->setValue(key + "/language_standard", getLanguageStandard());
}
