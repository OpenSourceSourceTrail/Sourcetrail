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
#include "IndexerPluginRegistry.h"
#include "language_packages.h"
#include "PlatformUserPaths.h"
#include "productVersion.h"
#include "ScopedFunctor.h"
#include "SourceGroupFactory.h"
#include "SourceGroupFactoryModuleCustom.h"
#include "SourceGroupFactoryModuleJava.h"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "UserPaths.h"
#include "Version.h"

#include "CxxToolchainRemote.h"
#include "ICxxToolchain.h"
#include "SourceGroupFactoryModuleCxx.h"    // BUILD_CXX_LANGUAGE_PACKAGE

namespace {

void signalHandler(int signum) {
  fmt::println("Interrupt signal received. {}", signum);
  MessageIndexingInterrupted{}.dispatch();
}

void addSourceGroupModules() {
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCustom>());
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleJava>());

  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCxx>());
  // Parsing a compilation database and compiling a precompiled header happen in the C/C++ indexer,
  // which is why this process links no language package. With no indexer installed the toolchain
  // simply answers nothing and the project stays browsable.
  ICxxToolchain::setInstance(std::make_shared<CxxToolchainRemote>());    // BUILD_CXX_LANGUAGE_PACKAGE

  IndexerPluginRegistry::getInstance()->discover();
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

  platform_paths::setupPaths();

  auto factory = std::make_shared<lib::Factory>();
  Application::createInstance(version, factory, nullptr, nullptr);
  [[maybe_unused]] const ScopedFunctor scopedFunctor([]() { Application::destroyInstance(); });

  // Must be constructed after Application::createInstance(), which is what sets up the
  // IMessageQueue singleton that ConsoleApplication registers itself with as a MessageListener.
  ConsoleApplication consoleApp;

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addSourceGroupModules();

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
