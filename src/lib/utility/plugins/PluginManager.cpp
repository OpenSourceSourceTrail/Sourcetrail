#include "PluginManager.h"
// STL
#include <filesystem>
// Boost
#include <boost/dll/import.hpp>
// fmt
#include <fmt/format.h>
// Qt5
#include <QTimer>
// internal
#include "IApplicationSettings.hpp"
#include "logging.h"

namespace fs = std::filesystem;
namespace dll = boost::dll;

constexpr std::string_view INSTANCE_NAME = "sInstance";

namespace plugin {

const PluginManager::Ptr& PluginManager::instance() {
  if(!s_pInstance) {
    s_pInstance = Ptr(new PluginManager);
  }

  return s_pInstance;
}

PluginManager::PluginManager() noexcept = default;

PluginManager::~PluginManager() = default;

void PluginManager::loadPlugins() {
  auto pluginsDir = IApplicationSettings::getInstanceRaw()->getPluginDir();
  if(!(fs::exists(pluginsDir) && fs::is_directory(pluginsDir))) {
    LOG_WARNING("Plugins dir is not exists");
    return;
  }

  // Call later
  QTimer::singleShot(0, [this, pluginsDir = std::move(pluginsDir)]() {
    for(auto file : fs::directory_iterator{pluginsDir}) {
      if(file.is_regular_file()) {
        loadPlugin(file.path().string());
      }
    }
  });
}

bool PluginManager::exists(const std::string& plugName) const {
  return mDynamicLibraries.find(plugName) != std::cend(mDynamicLibraries);
}

void PluginManager::loadPlugin(std::string_view pluginPath) {
  const auto pluginName = fs::path{pluginPath}.stem().string();
  if(exists(pluginName)) {
    LOG_WARNING(fmt::format("{} is loaded.", pluginPath.data()));
    return;
  }

  IPluginPtr pPlugin = dll::import_symbol<IPlugin>(pluginPath.data(), INSTANCE_NAME.data());
  if(pPlugin) {
    mDynamicLibraries.insert({pluginName, pPlugin});
    emit pluginLoaded(pluginName);
  }
}

}    // namespace plugin