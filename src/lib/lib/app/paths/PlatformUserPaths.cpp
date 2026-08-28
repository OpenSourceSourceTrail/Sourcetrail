#include "app/paths/PlatformUserPaths.h"

#include <cstdlib>
#include <filesystem>

#if defined(_WIN32)
#  include <windows.h>
#elif defined(__APPLE__)
#  include <vector>

#  include <mach-o/dyld.h>
#endif

#include "app/paths/AppPath.h"
#include "app/paths/UserPaths.h"
#include "FileSystem.h"
#include "utilityApp.h"

namespace {

// std::getenv is flagged unsafe by MSVC; the returned pointer is read immediately and not retained.
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
const char* readEnv(const char* name) {
  return std::getenv(name);
}
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

/**
 * Mirrors includesWindows.h: prefer a portable "user/" directory next to the executable, else
 * %LOCALAPPDATA%/Coati Software/Sourcetrail[ 64-bit]/, else a "user_fallback/" next to the exe.
 */
#if defined(_WIN32)
FilePath resolveUserDataPath(const FilePath& sharedDataPath) {
  FilePath userDataPath = sharedDataPath.getConcatenated(L"user/");
  if(userDataPath.exists()) {
    return userDataPath;
  }

  FilePath userLocalPath;
  if(const char* localAppData = readEnv("LOCALAPPDATA"); localAppData != nullptr) {
    userLocalPath = FilePath(std::string(localAppData));
  }
  if(!userLocalPath.exists()) {
    if(const char* appData = readEnv("APPDATA"); appData != nullptr) {
      userLocalPath = FilePath(std::string(appData) + "/../local");
    }
  }

  if(userLocalPath.exists()) {
    userDataPath = userLocalPath.getConcatenated(L"Coati Software/");
    if(utility::getApplicationArchitectureType() == ApplicationArchitectureType::X86_64) {
      userDataPath.concatenate(L"Sourcetrail 64-bit/");
    } else {
      userDataPath.concatenate(L"Sourcetrail/");
    }
    userDataPath.makeCanonical();
    return userDataPath;
  }

  return sharedDataPath.getConcatenated(L"user_fallback/");
}
#elif defined(__APPLE__)
FilePath resolveUserDataPath(const FilePath& /*sharedDataPath*/) {
  if(const char* home = readEnv("HOME"); home != nullptr) {
    return FilePath(std::string(home) + "/Library/Application Support/Sourcetrail/");
  }
  return FilePath(L"./user/").getAbsolute();
}
#else
FilePath resolveUserDataPath(const FilePath& /*sharedDataPath*/) {
  if(const char* home = readEnv("HOME"); home != nullptr) {
    return FilePath(std::string(home) + "/.config/sourcetrail/");
  }
  return FilePath(L"./user/").getAbsolute();
}
#endif

}    // namespace

namespace platform_paths {

FilePath getExecutableDirectory() {
#if defined(_WIN32)
  std::wstring buffer(MAX_PATH, L'\0');
  DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  // A truncated path yields ERROR_INSUFFICIENT_BUFFER; grow until it fits.
  while(length == buffer.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    buffer.resize(buffer.size() * 2, L'\0');
    length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  }
  if(length > 0) {
    buffer.resize(length);
    return FilePath(std::filesystem::path(buffer).parent_path().wstring());
  }
#elif defined(__linux__)
  std::error_code errorCode;
  if(const auto path = std::filesystem::canonical("/proc/self/exe", errorCode); !errorCode) {
    return FilePath(path.parent_path().wstring());
  }
#elif defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::vector<char> buffer(size + 1, '\0');
  if(_NSGetExecutablePath(buffer.data(), &size) == 0) {
    std::error_code errorCode;
    if(const auto path = std::filesystem::canonical(buffer.data(), errorCode); !errorCode) {
      return FilePath(path.parent_path().wstring());
    }
  }
#endif
  return FilePath(std::filesystem::current_path().wstring());
}

void setupPaths() {
  const FilePath appPath = getExecutableDirectory().getConcatenated(L"/").getAbsolute();

  AppPath::setSharedDataDirectoryPath(appPath);

  // Check if bundled as Linux AppImage, which puts shared data one level up under share/.
  if(appPath.getConcatenated(L"../share/data").exists()) {
    AppPath::setSharedDataDirectoryPath(appPath.getConcatenated(L"../share").getAbsolute());
  }

  const FilePath userDataPath = resolveUserDataPath(AppPath::getSharedDataDirectoryPath());
  UserPaths::setUserDataDirectoryPath(userDataPath);

  if(!userDataPath.exists()) {
    FileSystem::createDirectory(userDataPath);
  }
}

}    // namespace platform_paths
