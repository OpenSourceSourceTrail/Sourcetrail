#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <fmt/core.h>

#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

#include "app/Application.h"
#include "app/paths/AppPath.h"
#include "app/paths/PlatformUserPaths.h"
#include "app/paths/UserPaths.h"
#include "data/storage/StorageCache.h"
#include "EngineHost.h"
#include "EngineHttpService.h"
#include "factory/impls/Factory.hpp"
#include "FilePath.h"
#include "language_packages.h"
#include "productVersion.h"
#include "Profiling.h"
#include "ScopedFunctor.h"
#include "settings/ApplicationSettingsPrefiller.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"
#include "Version.h"

namespace {

constexpr uint16_t DefaultEnginePort = 54321;

// Signal handler sets this flag; main loop checks it.
volatile std::sig_atomic_t gStopRequested = 0;

void signalHandler(int /*signum*/) {
  gStopRequested = 1;
}

}    // namespace

// argv: [--port <port>]
int main(int argc, char* argv[]) {
  const profiling::Scope tracyScope{profiling::DefaultPort};

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
  engine_host::registerSourceGroupModules();

  // StorageCache IS the StorageAccess in the engine process — Application owns it.
  StorageCache* storageAccess = Application::getInstance()->getStorageCache();

  // Must come after createInstance: the publisher registers with IMessageQueue, and the dialog-view
  // factory is what Application hands to Project instead of the do-nothing base DialogView.
  engine_host::HttpEndpoint endpoint(storageAccess, /*broadcastOnly=*/false);
  endpoint.service().setShutdownHandler([]() { gStopRequested = 1; });

  const uint16_t assignedPort = endpoint.start(port);
  if(assignedPort == 0) {
    fmt::println(stderr, "Failed to start the HTTP engine server on 127.0.0.1:{}", port);
    return EXIT_FAILURE;
  }

  fmt::println("Sourcetrail_engine listening on 127.0.0.1:{}", assignedPort);
  std::cout.flush();

  std::ignore = std::signal(SIGINT, signalHandler);
  std::ignore = std::signal(SIGTERM, signalHandler);

  while(gStopRequested == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  endpoint.stop();

  return EXIT_SUCCESS;
}
