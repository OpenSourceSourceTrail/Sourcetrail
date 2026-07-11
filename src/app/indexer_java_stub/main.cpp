#include <cstdlib>
#include <stdexcept>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "ApplicationSettings.h"
#include "AppPath.h"
#include "GrpcIndexer.h"
#include "IApplicationSettings.hpp"
#include "LanguagePackageJavaStub.h"
#include "LanguagePackageManager.h"
#include "logging.h"
#include "UserPaths.h"

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
  spdlog::set_default_logger(std::make_shared<spdlog::logger>("indexer_java_stub", std::move(fileSink)));
}

void suppressCrashMessage() {
#ifdef _WIN32
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
}
}    // namespace

// Stand-in worker for INDEXER_COMMAND_JAVA, proving plugin discovery -> launch -> gRPC round
// trip end to end. Not the real Java parser (see docs/plan: that is a pure-JVM gRPC client,
// a later increment). Same argv contract as Sourcetrail_indexer:
// <processId> --engine-endpoint <endpoint> <appPath> <userDataPath> [logFilePath]
int main(int argc, char* argv[]) {
  if(argc < 6) {
    LOG_ERROR(
        "Usage: Sourcetrail_indexer_java_stub <processId> --engine-endpoint <endpoint> <appPath> <userDataPath> "
        "[logFilePath]");
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

  LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageJavaStub>());

  try {
    GrpcIndexer indexer(engineEndpoint, static_cast<Id>(processId));
    indexer.work();
  } catch(const std::runtime_error& error) {
    LOG_ERROR(error.what());
    return EXIT_FAILURE;
  } catch(...) {
    LOG_ERROR("Unknown error in indexer_java_stub");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
