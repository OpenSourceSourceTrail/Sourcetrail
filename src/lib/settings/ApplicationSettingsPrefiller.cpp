#include "ApplicationSettingsPrefiller.h"

#include <range/v3/to_container.hpp>
#include <range/v3/view/transform.hpp>

#include "IApplicationSettings.hpp"
#include "logging.h"
#include "type/MessageStatus.h"
#include "utility.h"
#include "utilityPathDetection.h"

void ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings* settings) {
  bool updated = false;

  updated |= prefillCxxHeaderPaths(settings);
  updated |= prefillCxxFrameworkPaths(settings);

  if(updated) {
    settings->save();
  }
}

bool ApplicationSettingsPrefiller::prefillCxxHeaderPaths(IApplicationSettings* settings) {
  if(settings->getHasPrefilledHeaderSearchPaths())    // allow empty
  {
    return false;
  }

  LOG_INFO("Prefilling header search paths");
  auto cxxHeaderDetector = utility::getCxxHeaderPathDetector();
  std::vector<FilePath> paths = cxxHeaderDetector->getPaths();
  if(!paths.empty()) {
    MessageStatus(L"Ran C/C++ header path detection, found " + std::to_wstring(paths.size()) + L" path" +
                  (paths.size() == 1 ? L"" : L"s"))
        .dispatch();

    settings->setHeaderSearchPaths(utility::toStlPath(paths));
  }

  settings->setHasPrefilledHeaderSearchPaths(true);
  return true;
}

bool ApplicationSettingsPrefiller::prefillCxxFrameworkPaths(IApplicationSettings* settings) {
  if(settings->getHasPrefilledFrameworkSearchPaths())    // allow empty
  {
    return false;
  }

  LOG_INFO("Prefilling framework search paths");
  std::shared_ptr<CombinedPathDetector> cxxFrameworkDetector = utility::getCxxFrameworkPathDetector();
  std::vector<FilePath> paths = cxxFrameworkDetector->getPaths();
  if(!paths.empty()) {
    MessageStatus(L"Ran C/C++ framework path detection, found " + std::to_wstring(paths.size()) + L" path" +
                  (paths.size() == 1 ? L"" : L"s"))
        .dispatch();

    settings->setFrameworkSearchPaths(utility::toStlPath(paths));
  }

  settings->setHasPrefilledFrameworkSearchPaths(true);
  return true;
}
