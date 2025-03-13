#include "SourceGroupSettingsWithCppStandard.h"

#include "ConfigManager.hpp"

std::wstring SourceGroupSettingsWithCppStandard::getDefaultCppStandardStatic() {
#ifdef __linux__
  return L"gnu++17";
#else
  return L"c++17";
#endif
}

SourceGroupSettingsWithCppStandard::~SourceGroupSettingsWithCppStandard() = default;

std::wstring SourceGroupSettingsWithCppStandard::getCppStandard() const {
  if(m_cppStandard.empty()) {
    return getDefaultCppStandard();
  }
  return m_cppStandard;
}

void SourceGroupSettingsWithCppStandard::setCppStandard(const std::wstring& standard) {
  m_cppStandard = standard;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::vector<std::wstring> SourceGroupSettingsWithCppStandard::getAvailableCppStandards() const {
  // as defined in clang/include/clang/Frontend/LangStandards.def

  // clang-format off
  return {
  L"c++23", L"gnu++23",
  L"c++2c", L"gnu++2c",
  L"c++20", L"gnu++20",
  L"c++17", L"gnu++17",
  L"c++14", L"gnu++14",
  L"c++11", L"gnu++11",
  L"c++03", L"gnu++03",
  L"c++98", L"gnu++98"};
  // clang-format on
}

bool SourceGroupSettingsWithCppStandard::equals(const SourceGroupSettingsBase* other) const {
  const auto* otherPtr = dynamic_cast<const SourceGroupSettingsWithCppStandard*>(other);

  return (nullptr != otherPtr && m_cppStandard == otherPtr->m_cppStandard);
}

void SourceGroupSettingsWithCppStandard::load(const ConfigManager* config, const std::string& key) {
  setCppStandard(config->getValueOrDefault<std::wstring>(key + "/cpp_standard", L""));
}

void SourceGroupSettingsWithCppStandard::save(ConfigManager* config, const std::string& key) {
  config->setValue(key + "/cpp_standard", getCppStandard());
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::wstring SourceGroupSettingsWithCppStandard::getDefaultCppStandard() const {
  return getDefaultCppStandardStatic();
}
