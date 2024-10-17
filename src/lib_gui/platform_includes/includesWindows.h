#pragma once

#include <string>

#include <QDir>

#include <type_traits>
#include <windows.h>

#include "Application.h"
#include "AppPath.h"
#include "FileSystem.h"
#include "logging.h"
#include "ResourcePaths.h"
#include "UserPaths.h"
#include "utility.h"
#include "utilityApp.h"

void setupPlatform([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {}

void setupApp([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  {
    HMODULE hModule = GetModuleHandleW(NULL);
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);

    std::wstring appPath(path);

    size_t pos = appPath.find_last_of(L"/");
    if(pos == std::wstring::npos) {
      pos = appPath.find_last_of(L"\\");
    }
    if(pos != std::wstring::npos) {
      appPath = appPath.substr(0, pos + 1);
    }
    AppPath::setSharedDataDirectoryPath(FilePath(appPath));
  }

  {
    FilePath userDataPath = AppPath::getSharedDataDirectoryPath().concatenate(L"user/");
    if(!userDataPath.exists()) {
#pragma warning(push)
#pragma warning(disable : 4996)
      FilePath userLocalPath = FilePath(std::string(std::getenv("LOCALAPPDATA")));
      if(!userLocalPath.exists()) {
        userLocalPath = FilePath(std::string(std::getenv("APPDATA")) + "/../local");
      }
#pragma warning(pop)

      if(userLocalPath.exists()) {
        userDataPath = userLocalPath.getConcatenated(L"Coati Software/");
        if(utility::getApplicationArchitectureType() == APPLICATION_ARCHITECTURE_X86_64) {
          userDataPath.concatenate(L"Sourcetrail 64-bit/");
        } else {
          userDataPath.concatenate(L"Sourcetrail/");
        }
        userDataPath.makeCanonical();
      } else {
        userDataPath = AppPath::getSharedDataDirectoryPath().concatenate(L"user_fallback/");
        LOG_ERROR_W(L"The \"%LOCALAPPDATA%\" path could not be found. Falling back to \"" + userDataPath.wstr() +
                    L"\" to store settings data.");
        FileSystem::createDirectory(userDataPath);
      }
    }

    UserPaths::setUserDataDirectoryPath(userDataPath);
  }

  // This "copyFile" method does nothing if the copy destination already exist
  FileSystem::copyFile(
      ResourcePaths::getFallbackDirectoryPath().concatenate(L"ApplicationSettings.xml"), UserPaths::getAppSettingsFilePath());
  FileSystem::copyFile(
      ResourcePaths::getFallbackDirectoryPath().concatenate(L"window_settings.ini"), UserPaths::getWindowSettingsFilePath());
}