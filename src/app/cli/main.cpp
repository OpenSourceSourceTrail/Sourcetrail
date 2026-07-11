// STL
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <fmt/core.h>
#include <fmt/format.h>

#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

#include "Application.h"
#include "ApplicationSettings.h"
#include "ApplicationSettingsPrefiller.h"
#include "AppPath.h"
#include "CommandLineParser.h"
#include "ConsoleApplication.h"
#include "FilePath.h"
#include "IApplicationSettings.hpp"
#include "impls/Factory.hpp"
#include "language_packages.h"
#include "LanguagePackageManager.h"
#include "productVersion.h"
#include "ScopedFunctor.h"
#include "SourceGroupFactory.h"
#include "SourceGroupFactoryModuleCustom.h"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "UserPaths.h"
#include "Version.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "LanguagePackageCxx.h"
#  include "SourceGroupFactoryModuleCxx.h"
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

namespace {

void signalHandler(int signum) {
  fmt::println("Interrupt signal received. {}", signum);
  MessageIndexingInterrupted{}.dispatch();
}

void addLanguagePackages() {
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCustom>());

#if BUILD_CXX_LANGUAGE_PACKAGE
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCxx>());
  LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageCxx>());
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
}

std::filesystem::path getExecutableDirectory() {
#if defined(__linux__)
  std::error_code errorCode;
  auto path = std::filesystem::canonical("/proc/self/exe", errorCode);
  if(!errorCode) {
    return path.parent_path();
  }
#endif
  return std::filesystem::current_path();
}

// Qt-free replacement for lib_gui's platform_includes setupApp()/setupPlatform().
void setupPaths() {
  const std::filesystem::path appPath = getExecutableDirectory();

  AppPath::setSharedDataDirectoryPath(FilePath(appPath.wstring() + L"/"));
  AppPath::setCxxIndexerDirectoryPath(FilePath(appPath.wstring() + L"/"));

  // Check if bundled as Linux AppImage
  if(const FilePath appImageShare = FilePath(appPath.wstring() + L"/../share/data"); appImageShare.exists()) {
    AppPath::setSharedDataDirectoryPath(FilePath(appPath.wstring() + L"/../share").getAbsolute());
  }

  std::filesystem::path userDataPath;
  if(const char* home = std::getenv("HOME"); home != nullptr) {
    userDataPath = std::filesystem::path(home) / ".config/sourcetrail/";
  }

  UserPaths::setUserDataDirectoryPath(FilePath(userDataPath.wstring()));

  std::error_code errorCode;
  std::filesystem::create_directories(userDataPath, errorCode);
}

}    // namespace

int main(int argc, char* argv[]) {
  // Disable logger as default till load it from settings
  if(auto* logger = spdlog::default_logger_raw(); nullptr != logger) {
    for(auto& sink : logger->sinks()) {
      sink->set_level(spdlog::level::level_enum::off);
    }
  }

  const Version version{VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH};

  commandline::CommandLineParser commandLineParser(version.toString());
  std::vector<std::string> args;
  if(argc > 1) {
    args = std::vector<std::string>(argv + 1, argv + argc);
  }

  commandLineParser.preparse(std::move(args));
  if(commandLineParser.exitApplication()) {
    return EXIT_SUCCESS;
  }

  IApplicationSettings::setInstance(std::make_shared<ApplicationSettings>());

  setupPaths();

  auto factory = std::make_shared<lib::Factory>();
  Application::createInstance(version, factory, nullptr, nullptr);
  [[maybe_unused]] const ScopedFunctor scopedFunctor([]() { Application::destroyInstance(); });

  // Must be constructed after Application::createInstance(), which is what sets up the
  // IMessageQueue singleton that ConsoleApplication registers itself with as a MessageListener.
  ConsoleApplication consoleApp;

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addLanguagePackages();

  std::ignore = std::signal(SIGINT, signalHandler);
  std::ignore = std::signal(SIGTERM, signalHandler);
  std::ignore = std::signal(SIGABRT, signalHandler);

  commandLineParser.parse();

  if(commandLineParser.hasError()) {
    std::wcout << commandLineParser.getError() << L'\n';
    return EXIT_FAILURE;
  }

  MessageLoadProject{commandLineParser.getProjectFilePath(),
                     false,
                     commandLineParser.getRefreshMode(),
                     commandLineParser.getShallowIndexingRequested()}
      .dispatch();

  consoleApp.run();

  return EXIT_SUCCESS;
}
