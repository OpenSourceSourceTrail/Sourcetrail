// STL
#include <csignal>
#include <cstdlib>
#include <iostream>

#include <fmt/format.h>

#include <spdlog/sinks/sink.h>

#include "Application.h"
#include "ApplicationSettings.h"
#include "ApplicationSettingsPrefiller.h"
#include "CommandLineParser.h"
#include "FilePath.h"
#include "IApplicationSettings.hpp"
#include "impls/Factory.hpp"
#include "includes.h"
#include "language_packages.h"
#include "LanguagePackageManager.h"
#include "logging.h"
#include "productVersion.h"
#include "QtApplication.h"
#include "QtCoreApplication.h"
#include "QtNetworkFactory.h"
#include "QtViewFactory.h"
#include "ResourcePaths.h"
#include "ScopedFunctor.h"
#include "SourceGroupFactory.h"
#include "SourceGroupFactoryModuleCustom.h"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "type/MessageStatus.h"
#include "utilityApp.h"
#include "utilityQt.h"
#include "utilityString.h"
#include "Version.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "LanguagePackageCxx.h"
#  include "SourceGroupFactoryModuleCxx.h"
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

namespace {
void signalHandler(int /*signum*/) {
  std::cout << "interrupt indexing\n";
  MessageIndexingInterrupted{}.dispatch();
}

void addLanguagePackages() {
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCustom>());

#if BUILD_CXX_LANGUAGE_PACKAGE
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCxx>());
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

#if BUILD_CXX_LANGUAGE_PACKAGE
  LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageCxx>());
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
}

void checkRunFromScript() {
#ifndef D_WINDOWS
  const auto expectedShareDirectory = FilePath(QCoreApplication::applicationDirPath().toStdWString() + L"/../share");
  if(qEnvironmentVariableIsEmpty("SOURCETRAIL_VIA_SCRIPT") && !expectedShareDirectory.exists()) {
    LOG_WARNING("Please run Sourcetrail via the Sourcetrail.sh script!");
  }
#endif
}

int runConsole(int argc, char** argv, const Version& version, commandline::CommandLineParser& commandLineParser) {
  const QtCoreApplication qtApp(argc, argv);

  checkRunFromScript();

  setupApp(argc, argv);

  auto factory = std::make_shared<lib::Factory>();
  Application::createInstance(version, factory, nullptr, nullptr);
  [[maybe_unused]] const ScopedFunctor scopedFunctor([]() { Application::destroyInstance(); });

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addLanguagePackages();

  // TODO(Hussein): Replace with Boost or Qt
  std::ignore = std::signal(SIGINT, signalHandler);
  std::ignore = std::signal(SIGTERM, signalHandler);
  std::ignore = std::signal(SIGABRT, signalHandler);

  commandLineParser.parse();

  if(commandLineParser.exitApplication()) {
    return EXIT_SUCCESS;
  }

  if(commandLineParser.hasError()) {
    std::wcout << commandLineParser.getError() << L'\n';
  } else {
    MessageLoadProject{commandLineParser.getProjectFilePath(),
                       false,
                       commandLineParser.getRefreshMode(),
                       commandLineParser.getShallowIndexingRequested()}
        .dispatch();
  }

  return QCoreApplication::exec();
}

int runGui(int argc, char** argv, const Version& version, commandline::CommandLineParser& commandLineParser) {
#ifdef D_WINDOWS
  {
    HWND consoleWnd = GetConsoleWindow();
    DWORD dwProcessId;
    GetWindowThreadProcessId(consoleWnd, &dwProcessId);
    if(GetCurrentProcessId() == dwProcessId) {    // Sourcetrail has not been started from console and thus has it's own console
      ShowWindow(consoleWnd, SW_HIDE);
    }
  }
#endif
  const QtApplication qtApp(argc, argv);

  checkRunFromScript();

  setupApp(argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QtApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  QtViewFactory viewFactory;
  QtNetworkFactory networkFactory;

  auto factory = std::make_shared<lib::Factory>();
  Application::createInstance(version, factory, &viewFactory, &networkFactory);
  [[maybe_unused]] const ScopedFunctor destroyApplication([]() { Application::destroyInstance(); });

  const auto message = fmt::format("Starting Sourcetrail {}bit, version {}", utility::getAppArchTypeString(), version.toString());
  MessageStatus{utility::decodeFromUtf8(message)}.dispatch();

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addLanguagePackages();

  // NOTE(Hussein): Extract to function
  utility::loadFontsFromDirectory(ResourcePaths::getFontsDirectoryPath(), L".otf");
  utility::loadFontsFromDirectory(ResourcePaths::getFontsDirectoryPath(), L".ttf");

  if(commandLineParser.hasError()) {
    Application::getInstance()->handleDialog(commandLineParser.getError());
  } else {
    MessageLoadProject{commandLineParser.getProjectFilePath(), false, RefreshMode::None}.dispatch();
  }

  return QApplication::exec();
}
}    // namespace

int main(int argc, char* argv[]) {
  // Disable logger as default till load it from settings
  if(auto* logger = spdlog::default_logger_raw(); nullptr != logger) {
    for(auto& sink : logger->sinks()) {
      sink->set_level(spdlog::level::level_enum::off);
    }
  }

  QCoreApplication::addLibraryPath(QStringLiteral("."));

  QApplication::setApplicationName(QStringLiteral("Sourcetrail"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#  ifdef D_LINUX
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#  endif
#endif

  const Version version(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  QApplication::setApplicationVersion(version.toString().c_str());

  commandline::CommandLineParser commandLineParser(version.toString());
  std::vector<std::string> args;
  if(argc > 1) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    args = std::vector<std::string>(static_cast<size_t>(argc - 1), argv[1]);
  }

  commandLineParser.preparse(std::move(args));
  if(commandLineParser.exitApplication()) {
    return EXIT_SUCCESS;
  }

  setupPlatform(argc, argv);

  IApplicationSettings::setInstance(std::make_shared<ApplicationSettings>());
  if(commandLineParser.runWithoutGUI()) {
    return runConsole(argc, argv, version, commandLineParser);
  }
  return runGui(argc, argv, version, commandLineParser);
}
