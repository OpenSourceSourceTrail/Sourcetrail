#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <fmt/core.h>

#include <grpcpp/server_builder.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

#include "Application.h"
#include "ApplicationSettings.h"
#include "ApplicationSettingsPrefiller.h"
#include "AppPath.h"
#include "CxxToolchainRemote.h"
#include "EngineEventPublisher.h"
#include "EngineServiceImpl.h"
#include "FilePath.h"
#include "IApplicationSettings.hpp"
#include "ICxxToolchain.h"
#include "impls/Factory.hpp"
#include "IndexerPluginRegistry.h"
#include "language_packages.h"
#include "PlatformUserPaths.h"
#include "productVersion.h"
#include "ScopedFunctor.h"
#include "SourceGroupFactory.h"
#include "SourceGroupFactoryModuleCustom.h"
#include "SourceGroupFactoryModuleCxx.h"
#include "SourceGroupFactoryModuleJava.h"
#include "StorageCache.h"
#include "UserPaths.h"
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

  EngineServiceImpl engineService(storageAccess);
  engineService.setShutdownHandler([]() { gStopRequested = 1; });

  // Must come after createInstance: the publisher registers with IMessageQueue, and the dialog-view
  // factory is what Application hands to Project instead of the do-nothing base DialogView.
  const EngineEventPublisher eventPublisher(&engineService);
  eventPublisher.installDialogViewFactory();

  grpc::ServerBuilder builder;
  // 127.0.0.1 rather than "localhost": on Windows the latter may resolve to ::1 first, leaving a
  // client that dialed the IPv4 loopback unable to connect.
  const std::string address = fmt::format("127.0.0.1:{}", port);
  int assignedPort = 0;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials(), &assignedPort);
  builder.RegisterService(&engineService);
  // Graph and file-content responses routinely exceed gRPC's 4 MB default on real projects.
  builder.SetMaxReceiveMessageSize(-1);
  builder.SetMaxSendMessageSize(-1);
  const std::unique_ptr<grpc::Server> server = builder.BuildAndStart();

  if(!server) {
    fmt::println(stderr, "Failed to start gRPC engine server on {}", address);
    return EXIT_FAILURE;
  }

  // Machine-readable handshake line, emitted first and flushed, so a parent process that spawned us
  // with "--port 0" can learn the ephemeral port. Keep this the very first line of stdout.
  fmt::println("ENGINE_PORT {}", assignedPort);
  std::cout.flush();
  fmt::println("Sourcetrail_engine listening on 127.0.0.1:{}", assignedPort);
  std::cout.flush();

  std::ignore = std::signal(SIGINT, signalHandler);
  std::ignore = std::signal(SIGTERM, signalHandler);

  while(gStopRequested == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  server->Shutdown();

  return EXIT_SUCCESS;
}
