#include <cstdlib>
#include <stdexcept>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "app/IndexerPluginRegistry.h"
#include "app/LanguagePackageManager.h"
#include "app/paths/AppPath.h"
#include "app/paths/UserPaths.h"
#include "indexing/logic/grpc/GrpcIndexer.h"
#include "language_packages.h"
#include "logging.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "CxxHelperMode.h"
#  include "LanguagePackageCxx.h"
#  include "project/CxxToolchainLocal.h"
#  include "project/logic/ICxxToolchain.h"
#endif

#ifdef _WIN32
#  include <Windows.h>
#endif

namespace {
void setupLogging(const std::string& logFilePath, spdlog::level::level_enum level) {
  for(auto& sink : spdlog::default_logger_raw()->sinks()) {
    sink->set_level(level);
  }
  auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath);
  fileSink->set_level(level);
  spdlog::set_default_logger(std::make_shared<spdlog::logger>("indexer", std::move(fileSink)));
}

void suppressCrashMessage() {
#ifdef _WIN32
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
}
}    // namespace

// argv: <processId> --engine-endpoint <endpoint> <appPath> <userDataPath> [logFilePath]
//   or: --helper <requestFile> <responseFile>
int main(int argc, char* argv[]) {
  if(argc >= 2 && std::string(argv[1]) == "--helper") {
    // Answering one toolchain question, for a process that has no Clang of its own.
    if(argc < 4) {
      LOG_ERROR("Usage: Sourcetrail_indexer --helper <requestFile> <responseFile>");
      return EXIT_FAILURE;
    }
#if BUILD_CXX_LANGUAGE_PACKAGE
    return helper::run(argv[2], argv[3]);
#else
    LOG_ERROR("This indexer was built without the C/C++ language package.");
    return EXIT_FAILURE;
#endif
  }

  if(argc < 6) {
    LOG_ERROR("Usage: Sourcetrail_indexer <processId> --engine-endpoint <endpoint> <appPath> <userDataPath> [logFilePath]");
    return EXIT_FAILURE;
  }

  int processId = -1;
  try {
    processId = std::stoi(argv[1]);
  } catch(const std::exception& e) {
    LOG_ERROR("Invalid processId: " + std::string(e.what()));
    return EXIT_FAILURE;
  }

  const std::string endpointFlag = argv[2];
  if(endpointFlag != "--engine-endpoint") {
    LOG_ERROR("Expected --engine-endpoint flag, got: " + endpointFlag);
    return EXIT_FAILURE;
  }

  const std::string engineEndpoint = argv[3];
  const std::string appPath = argv[4];
  const std::string userDataPath = argv[5];
  const std::string logFilePath = (argc >= 7) ? argv[6] : "";

  AppPath::setSharedDataDirectoryPath(FilePath(appPath));
  UserPaths::setUserDataDirectoryPath(FilePath(userDataPath));

  suppressCrashMessage();

  IApplicationSettings::setInstance(std::make_shared<ApplicationSettings>());
  auto* appSettings = IApplicationSettings::getInstanceRaw();
  if(!appSettings->load(UserPaths::getAppSettingsFilePath())) {
    LOG_ERROR("Failed to load application settings");
    return EXIT_FAILURE;
  }

  if(appSettings->getVerboseIndexerLoggingEnabled() && !logFilePath.empty()) {
    setupLogging(logFilePath, spdlog::level::level_enum(appSettings->getLoggingLevel()));
  } else {
    for(auto& sink : spdlog::default_logger_raw()->sinks()) {
      sink->set_level(spdlog::level::off);
    }
  }

  LOG_INFO(L"sharedDataPath: " + AppPath::getSharedDataDirectoryPath().wstr());
  LOG_INFO(L"userDataPath: " + UserPaths::getUserDataDirectoryPath().wstr());
  LOG_INFO("engineEndpoint: " + engineEndpoint);

#if BUILD_CXX_LANGUAGE_PACKAGE
  LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageCxx>());
  ICxxToolchain::setInstance(std::make_shared<CxxToolchainLocal>());
#endif

  IndexerPluginRegistry::getInstance()->discover();

  try {
    GrpcIndexer indexer(engineEndpoint, static_cast<Id>(processId));
    indexer.work();
  } catch(const std::runtime_error& error) {
    LOG_ERROR(error.what());
    return EXIT_FAILURE;
  } catch(...) {
    LOG_ERROR("Unknown error in indexer");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
