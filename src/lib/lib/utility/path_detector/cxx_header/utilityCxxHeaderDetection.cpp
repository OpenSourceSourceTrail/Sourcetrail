#include "utility/path_detector/cxx_header/utilityCxxHeaderDetection.h"

#ifdef _WIN32
#  include <windows.h>
#endif

#include "FileSystem.h"
#include "utilityApp.h"
#include "utilityString.h"

namespace utility {
std::vector<std::wstring> getCxxHeaderPaths(const std::string& compilerName) {
  std::vector<std::wstring> paths;

  const utility::ProcessOutput out = utility::executeProcess(
      utility::decodeFromUtf8(compilerName), {L"-x", L"c++", L"-v", L"-E", L"/dev/null"});
  if(out.exitCode == 0) {
    std::wstring standardHeaders = utility::substrBetween<std::wstring>(
        out.output, L"#include <...> search starts here:\n", L"\nEnd of search list");

    if(!standardHeaders.empty()) {
      for(const std::wstring& s : utility::splitToVector(standardHeaders, L'\n')) {
        paths.push_back(utility::trim(s));
      }
    }
  }

  return paths;
}

std::vector<FilePath> getWindowsSdkHeaderSearchPaths(ApplicationArchitectureType architectureType) {
  std::vector<FilePath> headerSearchPaths;

  std::vector<std::string> windowsSdkVersions;
  windowsSdkVersions.push_back("v8.1A");
  windowsSdkVersions.push_back("v8.1");
  windowsSdkVersions.push_back("v8.0A");
  windowsSdkVersions.push_back("v7.1A");
  windowsSdkVersions.push_back("v7.0A");

  for(size_t i = 0; i < windowsSdkVersions.size(); i++) {
    const FilePath sdkPath = getWindowsSdkRootPathUsingRegistry(architectureType, windowsSdkVersions[i]);
    if(sdkPath.exists()) {
      const FilePath sdkIncludePath = sdkPath.getConcatenated(L"include/");
      if(sdkIncludePath.exists()) {
        bool usingSubdirectories = false;
        for(const std::wstring subDirectory : {L"shared", L"um", L"winrt"}) {
          const FilePath sdkSubdirectory = sdkIncludePath.getConcatenated(subDirectory);
          if(sdkSubdirectory.exists()) {
            headerSearchPaths.push_back(sdkSubdirectory);
            usingSubdirectories = true;
          }
        }

        if(!usingSubdirectories) {
          headerSearchPaths.push_back(sdkIncludePath);
        }
        break;
      }
    }
  }
  {
    const FilePath sdkPath = getWindowsSdkRootPathUsingRegistry(architectureType, "v10.0");
    if(sdkPath.exists()) {
      for(const FilePath& versionPath : FileSystem::getDirectSubDirectories(sdkPath.getConcatenated(L"include/"))) {
        const FilePath ucrtPath = versionPath.getConcatenated(L"ucrt");
        if(ucrtPath.exists()) {
          headerSearchPaths.push_back(ucrtPath);
          break;
        }
      }
    }
  }

  return headerSearchPaths;
}

FilePath getWindowsSdkRootPathUsingRegistry([[maybe_unused]] ApplicationArchitectureType architectureType,
                                            [[maybe_unused]] const std::string& sdkVersion) {
#ifdef _WIN32
  std::string subKey = "SOFTWARE\\";
  if(architectureType == ApplicationArchitectureType::X86_32) {
    subKey += "Wow6432Node\\";
  }
  subKey += "Microsoft\\Microsoft SDKs\\Windows\\" + sdkVersion;

  char installationFolder[MAX_PATH] = {};
  DWORD installationFolderSize = sizeof(installationFolder);
  const LSTATUS status = RegGetValueA(
      HKEY_LOCAL_MACHINE, subKey.c_str(), "InstallationFolder", RRF_RT_REG_SZ, nullptr, installationFolder, &installationFolderSize);

  if(status == ERROR_SUCCESS) {
    FilePath path(utility::decodeFromUtf8(installationFolder));
    if(path.exists()) {
      return path;
    }
  }
#endif

  return FilePath();
}
}    // namespace utility
