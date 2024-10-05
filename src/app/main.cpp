// STL
#include <csignal>
#include <cstdlib>
#include <iostream>

#include <fmt/format.h>

#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "Application.h"
#include "ApplicationSettingsPrefiller.h"
#include "CommandLineParser.h"
#include "FilePath.h"
#include "IApplicationSettings.hpp"
#include "LanguagePackageManager.h"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "type/MessageStatus.h"
#include "QtApplication.h"
#include "QtCoreApplication.h"
#include "QtNetworkFactory.h"
#include "QtViewFactory.h"
#include "ResourcePaths.h"
#include "ScopedFunctor.h"
#include "SourceGroupFactory.h"
#include "SourceGroupFactoryModuleCustom.h"
#include "Version.h"
#include "impls/Factory.hpp"
#include "includes.h"
#include "language_packages.h"
#include "logging.h"
#include "productVersion.h"
#include "utilityApp.h"
#include "utilityQt.h"
#include "utilityString.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "LanguagePackageCxx.h"
#  include "SourceGroupFactoryModuleCxx.h"
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

void signalHandler(int /*signum*/) {
  std::cout << "interrupt indexing\n";
  MessageIndexingInterrupted().dispatch();
}

void setupLogging() {
  std::vector<spdlog::sink_ptr> sinkList;

  try {
    if(qEnvironmentVariableIsEmpty("ST_DISABLE_LOG_CONSOLE")) {
      auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      consoleSink->set_level(spdlog::level::trace);
      sinkList.emplace_back(std::move(consoleSink));
    }

    if(auto logFileEnv = qgetenv("ST_LOG_FILE"); !logFileEnv.isEmpty()) {
      auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFileEnv.toStdString(), true);
      fileSink->set_level(spdlog::level::trace);
      sinkList.emplace_back(std::move(fileSink));
    }

    spdlog::set_default_logger(std::make_shared<spdlog::logger>("multi_sink", sinkList.begin(), sinkList.end()));
  } catch(const spdlog::spdlog_ex& ex) {
    fmt::print(stderr, "{}\n", ex.what());
  }
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
#ifndef _WIN32
  const auto expectedShareDirectory = FilePath(QCoreApplication::applicationDirPath().toStdWString() + L"/../share");
  if(qEnvironmentVariableIsEmpty("SOURCETRAIL_VIA_SCRIPT") && !expectedShareDirectory.exists()) {
    LOG_WARNING("Please run Sourcetrail via the Sourcetrail.sh script!");
  }
#endif
}

int runConsole(int argc, char** argv, const Version& version, commandline::CommandLineParser& commandLineParser) {
  QtCoreApplication qtApp(argc, argv);

  checkRunFromScript();

  setupApp(argc, argv);

  setupLogging();

  auto factory = std::make_shared<lib::Factory>();
  Application::createInstance(version, factory, nullptr, nullptr);
  [[maybe_unused]] ScopedFunctor scopedFunctor([]() { Application::destroyInstance(); });

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addLanguagePackages();

  // TODO(Hussein): Replace with Boost or Qt
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGABRT, signalHandler);

  commandLineParser.parse();

  if(commandLineParser.exitApplication()) {
    return EXIT_SUCCESS;
  }

  if(commandLineParser.hasError()) {
    std::wcout << commandLineParser.getError() << std::endl;
  } else {
    MessageLoadProject(commandLineParser.getProjectFilePath(),
                       false,
                       commandLineParser.getRefreshMode(),
                       commandLineParser.getShallowIndexingRequested())
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
  QtApplication qtApp(argc, argv);

  checkRunFromScript();

  setupApp(argc, argv);

  setupLogging();

  QtApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  QtViewFactory viewFactory;
  QtNetworkFactory networkFactory;

  auto factory = std::make_shared<lib::Factory>();
  Application::createInstance(version, factory, &viewFactory, &networkFactory);
  [[maybe_unused]] ScopedFunctor destroyApplication([]() { Application::destroyInstance(); });

  const auto message = fmt::format("Starting Sourcetrail {}bit, version {}", utility::getAppArchTypeString(), version.toString());
  MessageStatus(utility::decodeFromUtf8(message)).dispatch();

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addLanguagePackages();

  // NOTE(Hussein): Extract to function
  utility::loadFontsFromDirectory(ResourcePaths::getFontsDirectoryPath(), L".otf");
  utility::loadFontsFromDirectory(ResourcePaths::getFontsDirectoryPath(), L".ttf");

  if(commandLineParser.hasError()) {
    Application::getInstance()->handleDialog(commandLineParser.getError());
  } else {
    MessageLoadProject(commandLineParser.getProjectFilePath(), false, REFRESH_NONE).dispatch();
  }

  return QApplication::exec();
}

int main(int argc, char* argv[]) {
  QCoreApplication::addLibraryPath(QStringLiteral("."));

  QApplication::setApplicationName(QStringLiteral("Sourcetrail"));

#ifdef D_LINUX
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
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
