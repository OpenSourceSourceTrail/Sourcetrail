#pragma once

#include <vector>

#include "FilePath.h"
#include "SourceGroupSettingsComponent.h"

class SourceGroupSettingsWithJavaClassPath : public SourceGroupSettingsComponent {
public:
  virtual ~SourceGroupSettingsWithJavaClassPath() = default;

  std::vector<FilePath> getClassPaths() const;
  std::vector<FilePath> getClassPathsExpandedAndAbsolute() const;
  void setClassPaths(const std::vector<FilePath>& classPaths);

  std::wstring getLanguageStandard() const;
  void setLanguageStandard(const std::wstring& languageStandard);

protected:
  bool equals(const SourceGroupSettingsBase* other) const override;

  void load(const ConfigManager* config, const std::string& key) override;
  void save(ConfigManager* config, const std::string& key) override;

private:
  std::vector<FilePath> m_classPaths;
  std::wstring m_languageStandard = L"17";
};
