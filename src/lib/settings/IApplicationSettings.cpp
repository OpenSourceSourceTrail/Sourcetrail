#include "IApplicationSettings.hpp"
#include "utility.h"

IApplicationSettings::Ptr IApplicationSettings::mInstance;

IApplicationSettings::IApplicationSettings() noexcept = default;
IApplicationSettings::~IApplicationSettings() noexcept = default;

bool IApplicationSettings::operator==(const IApplicationSettings& other) const noexcept {
  return utility::isPermutation(getHeaderSearchPaths(), other.getHeaderSearchPaths()) &&
      utility::isPermutation(getFrameworkSearchPaths(), other.getFrameworkSearchPaths());
}
