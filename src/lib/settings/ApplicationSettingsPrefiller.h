#pragma once

struct IApplicationSettings;

class ApplicationSettingsPrefiller final {
public:
  static void prefillPaths(IApplicationSettings* settings);

private:
  static bool prefillCxxHeaderPaths(IApplicationSettings* settings);
  static bool prefillCxxFrameworkPaths(IApplicationSettings* settings);
};