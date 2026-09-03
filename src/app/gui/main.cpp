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
#include "data/storage/StorageCache.h"
#include "EngineCall.h"
#include "EngineChannel.h"
#include "EngineEventClient.h"
#include "EngineHost.h"
#include "factory/impls/Factory.hpp"
#include "FilePath.h"
#include "HttpStorageAccess.h"
#include "ide_communication/ui/QtNetworkFactory.h"
#include "includes.h"
#include "indexing/messages/MessageIndexingInterrupted.h"
#include "logging.h"
#include "productVersion.h"
#include "Profiling.h"
#include "project/messages/MessageLoadProject.h"
#include "qt/engine/QtEngineSupervisor.h"
#include "qt/QtApplication.h"
#include "qt/QtCoreApplication.h"
#include "qt/utility/utilityQt.h"
#include "qt/view/QtViewFactory.h"
#include "ScopedFunctor.h"
#include "settings/ApplicationSettingsPrefiller.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"
#include "status/messages/MessageStatus.h"
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

/**
 * Builds the factory for a remote engine: either attaching to the one named on the command line, or
 * -- with a bare --engine -- spawning and supervising a private one, which is what the GUI used to
 * do unconditionally.
 *
 * `channel` and `storageAccess` outlive the returned factory's use, so both are owned by the caller.
 */
std::shared_ptr<lib::IFactory> makeRemoteFactory(const commandline::CommandLineParser& commandLineParser,
                                                 QtEngineSupervisor& supervisor,
                                                 std::unique_ptr<EngineChannel>& channel,
                                                 std::shared_ptr<HttpStorageAccess>& storageAccess) {
  const std::string& endpoint = commandLineParser.getEngineEndpoint();
  if(endpoint.empty()) {
    return startEngineAndMakeFactory(supervisor, storageAccess);
  }

  // "host:port,token" -- the two halves of the handshake line the engine printed. Attaching means
  // nobody supervises that engine: it was already running and is not ours to restart.
  const auto separator = endpoint.rfind(',');
  if(separator == std::string::npos) {
    LOG_ERROR(fmt::format("--engine expects \"host:port,token\", got \"{}\"", endpoint));
    return nullptr;
  }
  channel = std::make_unique<EngineChannel>(endpoint.substr(0, separator), endpoint.substr(separator + 1));
  storageAccess = std::make_shared<HttpStorageAccess>(channel.get());
  return std::make_shared<client::ClientFactory>(channel.get(), storageAccess);
}

/**
 * Hosts the engine in this process and keeps it alive for as long as the returned handle lives.
 *
 * The endpoint exists even without --http-port: the read path needs no server, but the parts of the
 * GUI that ask the engine questions rather than reading the index -- what it can index, what a
 * compilation database contains -- go through the same client calls a remote engine would answer,
 * and with no channel those calls need a local dispatcher or they find no engine at all. A listener
 * is opened only when a port was asked for, for clients such as the MCP server.
 */
std::unique_ptr<engine_host::HttpEndpoint> hostEngine(const commandline::CommandLineParser& commandLineParser) {
  // broadcastOnly: the publisher's DialogView factory answers dialogs over the wire, which would
  // replace the real Qt dialogs this process has. Only the event broadcast is wanted here.
  auto endpoint = std::make_unique<engine_host::HttpEndpoint>(Application::getInstance()->getStorageCache(),
                                                              /*broadcastOnly=*/true);
  client::setLocalDispatch(
      [endpoint = endpoint.get()](const std::string& method, const std::string& target, const std::string& body) {
        return endpoint->callLocal(method, target, body);
      });

  if(const auto port = commandLineParser.getHttpPort(); port.has_value() && endpoint->start(*port) == 0) {
    LOG_ERROR(fmt::format("Failed to serve the engine HTTP API on 127.0.0.1:{}", *port));
  }
  return endpoint;
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
  std::unique_ptr<EngineChannel> channel;
  std::unique_ptr<client::EngineEventClient> eventClient;
  std::unique_ptr<engine_host::HttpEndpoint> httpEndpoint;

  if(commandLineParser.usesRemoteEngine()) {
    auto factory = makeRemoteFactory(commandLineParser, supervisor, channel, storageAccess);
    if(!factory) {
      return EXIT_FAILURE;
    }
    Application::createInstance(version, std::move(factory), nullptr, nullptr);
    // Indexing progress, status lines and "the index is ready" all originate on the engine's message
    // bus; this is the only thing that carries them across. Started after createInstance, which is
    // what sets up the local IMessageQueue it dispatches onto.
    eventClient = std::make_unique<client::EngineEventClient>(channel ? channel.get() : supervisor.getChannel());
    eventClient->start();
  } else {
    // In-process: lib::Factory builds the real Project, which owns PersistentStorage and points the
    // StorageCache at it. Every progress message is already on this process's bus, so there is
    // nothing to carry across and no EngineEventClient.
    Application::createInstance(version, std::make_shared<lib::Factory>(), nullptr, nullptr);
    engine_host::registerSourceGroupModules();
  }
  [[maybe_unused]] const ScopedFunctor scopedFunctor([]() {
    client::setLocalDispatch(nullptr);
    Application::destroyInstance();
  });

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());

  if(!commandLineParser.usesRemoteEngine()) {
    httpEndpoint = hostEngine(commandLineParser);
  }

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
  std::unique_ptr<EngineChannel> channel;
  std::unique_ptr<client::EngineEventClient> eventClient;
  std::unique_ptr<engine_host::HttpEndpoint> httpEndpoint;

  if(commandLineParser.usesRemoteEngine()) {
    auto factory = makeRemoteFactory(commandLineParser, supervisor, channel, storageAccess);
    if(!factory) {
      return EXIT_FAILURE;
    }
    Application::createInstance(version, std::move(factory), &viewFactory, &networkFactory);
    // See runConsole: without this the GUI never hears back from the engine, so nothing refreshes
    // after a load and indexing progress stays at 0%.
    eventClient = std::make_unique<client::EngineEventClient>(channel ? channel.get() : supervisor.getChannel());
    eventClient->start();
  } else {
    Application::createInstance(version, std::make_shared<lib::Factory>(), &viewFactory, &networkFactory);
    engine_host::registerSourceGroupModules();
  }
  [[maybe_unused]] const ScopedFunctor destroyApplication([]() {
    client::setLocalDispatch(nullptr);
    Application::destroyInstance();
  });

  if(!commandLineParser.usesRemoteEngine()) {
    httpEndpoint = hostEngine(commandLineParser);
  }

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
  // Opens the Tracy client; a no-op unless the build was configured with -DENABLE_TRACY=ON. First
  // statement in main because a zone reached before it is undefined behaviour.
  const profiling::Scope tracyScope{profiling::DefaultPort};

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
    args = std::vector<std::string>(argv + 1, argv + argc);
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
