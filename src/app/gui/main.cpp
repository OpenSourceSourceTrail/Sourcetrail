// STL
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <fmt/core.h>
#include <fmt/format.h>

#include <spdlog/sinks/sink.h>
#include <system_error>

#include "app/Application.h"
#include "app/paths/ResourcePaths.h"
#include "ClientFactory.h"
#include "CommandLineParser.h"
#include "EngineEventClient.h"
#include "FilePath.h"
#include "HttpStorageAccess.h"
#include "includes.h"
#include "logging.h"
#include "productVersion.h"
#include "qt/engine/QtEngineSupervisor.h"
#include "qt/network/QtNetworkFactory.h"
#include "qt/QtApplication.h"
#include "qt/QtCoreApplication.h"
#include "qt/utility/utilityQt.h"
#include "qt/view/QtViewFactory.h"
#include "ScopedFunctor.h"
#include "settings/ApplicationSettingsPrefiller.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "type/MessageStatus.h"
#include "utilityApp.h"
#include "utilityString.h"
#include "Version.h"

namespace {
void signalHandler(int signum) {
  fmt::println("Interrupt signal received. {}", signum);
  MessageIndexingInterrupted{}.dispatch();
}

/**
 * Starts the engine and returns the factory an Application needs to talk to it.
 *
 * The supervisor outlives the returned factory's use, so it is created by the caller; everything
 * here is deliberately non-fatal -- if the engine never comes up the GUI still runs, showing empty
 * views, which is the whole point of routing storage through HttpStorageAccess.
 */
std::shared_ptr<lib::IFactory> startEngineAndMakeFactory(QtEngineSupervisor& supervisor,
                                                         std::shared_ptr<HttpStorageAccess>& storageAccess) {
  supervisor.start();
  storageAccess = std::make_shared<HttpStorageAccess>(supervisor.getChannel());
  return std::make_shared<client::ClientFactory>(supervisor.getChannel(), storageAccess);
}

void checkRunFromScript() {
#ifndef D_WINDOWS
  std::error_code errorCode;
  const auto expectedShareDirectory = std::filesystem::path{QCoreApplication::applicationDirPath().toStdString()}.parent_path() /
      "share";
  if(qEnvironmentVariableIsEmpty("SOURCETRAIL_VIA_SCRIPT") && !std::filesystem::exists(expectedShareDirectory, errorCode)) {
    LOG_WARNING("Please run Sourcetrail via the Sourcetrail.sh script!");
  }
#endif
}

int runConsole(int argc, char** argv, const Version& version, commandline::CommandLineParser& commandLineParser) {
  const QtCoreApplication qtApp(argc, argv);

  checkRunFromScript();

  setupApp(argc, argv);

  QtEngineSupervisor supervisor;
  std::shared_ptr<HttpStorageAccess> storageAccess;
  Application::createInstance(version, startEngineAndMakeFactory(supervisor, storageAccess), nullptr, nullptr);
  [[maybe_unused]] const ScopedFunctor scopedFunctor([]() { Application::destroyInstance(); });

  // Indexing progress, status lines and "the index is ready" all originate on the engine's message
  // bus; this is the only thing that carries them across. Started after createInstance, which is
  // what sets up the local IMessageQueue it dispatches onto.
  client::EngineEventClient eventClient(supervisor.getChannel());
  eventClient.start();

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());

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

  QtEngineSupervisor supervisor;
  std::shared_ptr<HttpStorageAccess> storageAccess;
  Application::createInstance(version, startEngineAndMakeFactory(supervisor, storageAccess), &viewFactory, &networkFactory);
  [[maybe_unused]] const ScopedFunctor destroyApplication([]() { Application::destroyInstance(); });

  // See runConsole: without this the GUI never hears back from the engine, so nothing refreshes
  // after a load and indexing progress stays at 0%.
  client::EngineEventClient eventClient(supervisor.getChannel());
  eventClient.start();

  const auto message = fmt::format("Starting Sourcetrail {}bit, version {}", utility::getAppArchTypeString(), version.toString());
  MessageStatus{utility::decodeFromUtf8(message)}.dispatch();

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());

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

  const Version version{VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH};
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
