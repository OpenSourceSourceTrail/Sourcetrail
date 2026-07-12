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
#include "EngineServiceImpl.h"
#include "FilePath.h"
#include "IApplicationSettings.hpp"
#include "impls/Factory.hpp"
#include "IndexerPluginRegistry.h"
#include "language_packages.h"
#include "LanguagePackageManager.h"
#include "ScopedFunctor.h"
#include "SourceGroupFactory.h"
#include "SourceGroupFactoryModuleCustom.h"
#include "SourceGroupFactoryModuleJava.h"
#include "StorageCache.h"
#include "UserPaths.h"
#include "Version.h"
#include "productVersion.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "LanguagePackageCxx.h"
#  include "SourceGroupFactoryModuleCxx.h"
#endif

namespace {

constexpr uint16_t DefaultEnginePort = 54321;

std::filesystem::path getExecutableDirectory() {
#if defined(__linux__)
  std::error_code ec;
  auto path = std::filesystem::canonical("/proc/self/exe", ec);
  if(!ec) {
    return path.parent_path();
  }
#endif
  return std::filesystem::current_path();
}

void setupPaths() {
  const std::filesystem::path appPath = getExecutableDirectory();
  AppPath::setSharedDataDirectoryPath(FilePath(appPath.wstring() + L"/"));
  AppPath::setCxxIndexerDirectoryPath(FilePath(appPath.wstring() + L"/"));

  if(const FilePath appImageShare = FilePath(appPath.wstring() + L"/../share/data"); appImageShare.exists()) {
    AppPath::setSharedDataDirectoryPath(FilePath(appPath.wstring() + L"/../share").getAbsolute());
  }

  std::filesystem::path userDataPath;
  if(const char* home = std::getenv("HOME"); home != nullptr) {
    userDataPath = std::filesystem::path(home) / ".config/sourcetrail/";
  }
  UserPaths::setUserDataDirectoryPath(FilePath(userDataPath.wstring()));

  std::error_code ec;
  std::filesystem::create_directories(userDataPath, ec);
}

void addLanguagePackages() {
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCustom>());
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleJava>());
#if BUILD_CXX_LANGUAGE_PACKAGE
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCxx>());
  LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageCxx>());
#endif

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

  setupPaths();

  auto factory = std::make_shared<lib::Factory>();
  Application::createInstance(version, factory, nullptr, nullptr);
  [[maybe_unused]] const ScopedFunctor scopedFunctor([]() { Application::destroyInstance(); });

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addLanguagePackages();

  // StorageCache IS the StorageAccess in the engine process — Application owns it.
  StorageCache* storageAccess = Application::getInstance()->getStorageCache();

  EngineServiceImpl engineService(storageAccess);

  grpc::ServerBuilder builder;
  const std::string address = fmt::format("localhost:{}", port);
  int assignedPort = 0;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials(), &assignedPort);
  builder.RegisterService(&engineService);
  const std::unique_ptr<grpc::Server> server = builder.BuildAndStart();

  if(!server) {
    fmt::println(stderr, "Failed to start gRPC engine server on {}", address);
    return EXIT_FAILURE;
  }

  fmt::println("Sourcetrail_engine listening on localhost:{}", assignedPort);
  std::cout.flush();

  std::ignore = std::signal(SIGINT, signalHandler);
  std::ignore = std::signal(SIGTERM, signalHandler);

  while(gStopRequested == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  server->Shutdown();

  return EXIT_SUCCESS;
}
