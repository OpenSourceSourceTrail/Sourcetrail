#include <cstdlib>
#include <iterator>
#include <stdexcept>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "ApplicationSettings.h"
#include "AppPath.h"
#include "IApplicationSettings.hpp"
#include "InterprocessIndexer.h"
#include "language_packages.h"
#include "LanguagePackageManager.h"
#include "logging.h"
#include "UserPaths.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "LanguagePackageCxx.h"
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

#ifdef _WIN32
#  include <Windows.h>
#endif

namespace {
void setupLogging(const std::string logFilePath) {
  auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath);
  fileSink->set_level(spdlog::level::trace);
  spdlog::set_default_logger(std::make_shared<spdlog::logger>("indexer", std::move(fileSink)));
}

void suppressCrashMessage() {
#ifdef _WIN32
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif    // _WIN32
}
}    // namespace

int main(int argc, char* argv[]) {
  int processId = -1;
  std::string instanceUuid;
  std::string appPath;
  std::string userDataPath;
  std::string logFilePath;

  if(argc >= 2) {
    try {
      processId = std::stoi(argv[1]);
    } catch(const std::invalid_argument& e) {
      LOG_ERROR("Invalid argument for processId: " + std::string(e.what()));
      return EXIT_FAILURE;
    } catch(const std::out_of_range& e) {
      LOG_ERROR("Argument out of range for processId: " + std::string(e.what()));
      return EXIT_FAILURE;
    }
  } else {
    LOG_ERROR("Process ID is not provided");
    return EXIT_FAILURE;
  }

  if(argc >= 3) {
    instanceUuid = argv[2];
  } else {
    LOG_ERROR("Instance UUID is not provided");
    return EXIT_FAILURE;
  }

  if(argc >= 4) {
    appPath = argv[3];
  } else {
    LOG_ERROR("App path is not provided");
    return EXIT_FAILURE;
  }

  if(argc >= 5) {
    userDataPath = argv[4];
  } else {
    LOG_ERROR("User data path is not provided");
    return EXIT_FAILURE;
  }

  if(argc >= 6) {
    logFilePath = argv[5];
  }

  AppPath::setSharedDataDirectoryPath(FilePath(appPath));
  UserPaths::setUserDataDirectoryPath(FilePath(userDataPath));

  if(!logFilePath.empty()) {
    setupLogging(std::string(logFilePath));
  }

  suppressCrashMessage();

  IApplicationSettings::setInstance(std::make_shared<ApplicationSettings>());
  auto* appSettings = IApplicationSettings::getInstanceRaw();
  if(!appSettings->load(UserPaths::getAppSettingsFilePath())) {
    LOG_ERROR("Failed to load application settings");
    return EXIT_FAILURE;
  }

  LOG_INFO_W(L"sharedDataPath: " + AppPath::getSharedDataDirectoryPath().wstr());
  LOG_INFO_W(L"userDataPath: " + UserPaths::getUserDataDirectoryPath().wstr());


#if BUILD_CXX_LANGUAGE_PACKAGE
  LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageCxx>());
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

  try {
    InterprocessIndexer indexer(instanceUuid, Id(processId));
    indexer.work();
  } catch(std::runtime_error& error) {
    LOG_ERROR(error.what());
    return EXIT_FAILURE;
  } catch(...) {
    LOG_ERROR("Unknown error");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
