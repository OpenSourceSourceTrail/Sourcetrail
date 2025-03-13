#pragma once
#include <vector>

#include "SourceGroupSettingsComponent.h"

class SourceGroupSettingsWithCppStandard : public SourceGroupSettingsComponent {
public:
  static std::wstring getDefaultCppStandardStatic();

  ~SourceGroupSettingsWithCppStandard() override;

  [[nodiscard]] std::wstring getCppStandard() const;
  void setCppStandard(const std::wstring& standard);

  [[nodiscard]] std::vector<std::wstring> getAvailableCppStandards() const;

protected:
  [[nodiscard]] bool equals(const SourceGroupSettingsBase* other) const override;

  void load(const ConfigManager* config, const std::string& key) override;
  void save(ConfigManager* config, const std::string& key) override;

private:
  [[nodiscard]] std::wstring getDefaultCppStandard() const;

  std::wstring m_cppStandard;
};
