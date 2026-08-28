#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <fmt/core.h>

#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

#include "app/Application.h"
#include "app/IndexerPluginRegistry.h"
#include "app/paths/AppPath.h"
#include "app/paths/PlatformUserPaths.h"
#include "app/paths/UserPaths.h"
#include "data/storage/StorageCache.h"
#include "EngineEventPublisher.h"
#include "EngineHttpService.h"
#include "factory/impls/Factory.hpp"
#include "FilePath.h"
#include "HttpServer.h"
#include "language_packages.h"
#include "productVersion.h"
#include "project/CxxToolchainRemote.h"
#include "project/ICxxToolchain.h"
#include "project/SourceGroupFactory.h"
#include "project/SourceGroupFactoryModuleCustom.h"
#include "project/SourceGroupFactoryModuleCxx.h"
#include "project/SourceGroupFactoryModuleJava.h"
#include "ScopedFunctor.h"
#include "settings/ApplicationSettingsPrefiller.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"
#include "utilityUuid.h"
#include "Version.h"

namespace {

constexpr uint16_t DefaultEnginePort = 54321;

void addSourceGroupModules() {
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCustom>());
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleJava>());
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCxx>());
  // Parsing a compilation database and compiling a precompiled header happen in the C/C++ indexer,
  // which is why this process links no language package. With no indexer installed the toolchain
  // simply answers nothing and the project stays browsable.
  ICxxToolchain::setInstance(std::make_shared<CxxToolchainRemote>());

  IndexerPluginRegistry::getInstance()->discover();
}

// Signal handler sets this flag; main loop checks it.
volatile std::sig_atomic_t gStopRequested = 0;

void signalHandler(int /*signum*/) {
  gStopRequested = 1;
}

}    // namespace

// argv: [--port <port>]
int main(int argc, char* argv[]) {
  if(auto* logger = spdlog::default_logger_raw(); logger != nullptr) {
    for(auto& sink : logger->sinks()) {
      sink->set_level(spdlog::level::level_enum::off);
    }
  }

  uint16_t port = DefaultEnginePort;
  for(int i = 1; i + 1 < argc; ++i) {
    if(std::string(argv[i]) == "--port") {
      try {
        port = static_cast<uint16_t>(std::stoi(argv[i + 1]));
      } catch(...) {
        fmt::println(stderr, "Invalid port: {}", argv[i + 1]);
        return EXIT_FAILURE;
      }
      ++i;
    }
  }

  const Version version{VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH};

  IApplicationSettings::setInstance(std::make_shared<ApplicationSettings>());

  platform_paths::setupPaths();

  auto factory = std::make_shared<lib::Factory>();
  Application::createInstance(version, factory, nullptr, nullptr);
  [[maybe_unused]] const ScopedFunctor scopedFunctor([]() { Application::destroyInstance(); });

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addSourceGroupModules();

  // StorageCache IS the StorageAccess in the engine process — Application owns it.
  StorageCache* storageAccess = Application::getInstance()->getStorageCache();

  EngineHttpService engineService(storageAccess);
  engineService.setShutdownHandler([]() { gStopRequested = 1; });

  // Must come after createInstance: the publisher registers with IMessageQueue, and the dialog-view
  // factory is what Application hands to Project instead of the do-nothing base DialogView.
  const EngineEventPublisher eventPublisher(&engineService);
  eventPublisher.installDialogViewFactory();

  // A loopback HTTP port is reachable by every process on the machine, and by any page the user's
  // browser loads -- which the gRPC port it replaces was not. The token is the proof of being the
  // client this engine was spawned for; it is handed over on the handshake line below, which only
  // the parent process can read.
  http::Server server(utility::getUuidString());
  engineService.registerRoutes(server);

  const uint16_t assignedPort = server.start(port);
  if(assignedPort == 0) {
    fmt::println(stderr, "Failed to start the HTTP engine server on 127.0.0.1:{}", port);
    return EXIT_FAILURE;
  }

  // Machine-readable handshake line, emitted first and flushed, so a parent process that spawned us
  // with "--port 0" can learn the ephemeral port and the token. Keep this the very first line of
  // stdout.
  fmt::println("ENGINE_PORT {} {}", assignedPort, server.authToken());
  std::cout.flush();
  fmt::println("Sourcetrail_engine listening on 127.0.0.1:{}", assignedPort);
  std::cout.flush();

  std::ignore = std::signal(SIGINT, signalHandler);
  std::ignore = std::signal(SIGTERM, signalHandler);

  while(gStopRequested == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // Releases anything blocked waiting for a client to answer a dialog before the threads go away.
  engineService.abortDialogs();
  server.stop();

  return EXIT_SUCCESS;
}
