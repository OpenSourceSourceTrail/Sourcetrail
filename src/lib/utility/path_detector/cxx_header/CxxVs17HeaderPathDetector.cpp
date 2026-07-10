#include "CxxVs17HeaderPathDetector.hpp"

#include <vector>

#include "FilePath.h"
#include "FileSystem.h"
#include "logging.h"
#include "utility.h"
#include "utilityApp.h"
#include "utilityCxxHeaderDetection.h"
#include "utilityString.h"


namespace {
constexpr int VSWHERE_TIMEOUT_MS = 10'000;
constexpr const wchar_t* VSWHERE_PATH = L"%ProgramFiles(x86)%/Microsoft Visual Studio/Installer/vswhere.exe";

std::optional<FilePath> findVsWhereExecutable() {
  try {
    const std::vector<FilePath> expandedPaths = FilePath(VSWHERE_PATH).expandEnvironmentVariables();
    if(expandedPaths.empty()) {
      LOG_WARNING(L"Failed to expand environment variables in vswhere.exe path");
      return std::nullopt;
    }

    const FilePath& vsWherePath = expandedPaths.front();
    // Validate the executable name to prevent command injection
    if(vsWherePath.fileName() != L"vswhere.exe") {
      LOG_ERROR(L"Unexpected executable name: " + vsWherePath.fileName());
      return std::nullopt;
    }

    if(!vsWherePath.exists()) {
      LOG_WARNING(L"vswhere.exe does not exist at: " + vsWherePath.wstr());
      return std::nullopt;
    }
    return vsWherePath;
  } catch(const std::exception& e) {
    LOG_ERROR(L"Exception while locating vswhere.exe: " + utility::decodeFromUtf8(e.what()));
    return std::nullopt;
  }
}

std::vector<FilePath> detectWindowsSdkHeaders() {
  std::vector<FilePath> sdkHeaders;
  try {
    // Detect system architecture
    ApplicationArchitectureType systemArch = utility::getApplicationArchitectureType();

    // logInfo(L"Detecting Windows SDK headers for architecture: " + utility::architectureToString(systemArch));

    // Try system architecture first
    sdkHeaders = utility::getWindowsSdkHeaderSearchPaths(systemArch);

    // Fallback strategy
    if(sdkHeaders.empty()) {
      LOG_WARNING(L"No SDK headers found for system architecture, trying x86_64");
      sdkHeaders = utility::getWindowsSdkHeaderSearchPaths(ApplicationArchitectureType::X86_64);
    }

    if(sdkHeaders.empty()) {
      LOG_WARNING(L"No SDK headers found for x86_64, trying x86_32");
      sdkHeaders = utility::getWindowsSdkHeaderSearchPaths(ApplicationArchitectureType::X86_32);
    }

    // Try ARM64 as last resort (Windows on ARM)
    if(sdkHeaders.empty()) {
      LOG_WARNING(L"No SDK headers found for x86, trying ARM64");
      sdkHeaders = utility::getWindowsSdkHeaderSearchPaths(ApplicationArchitectureType::ARM);
    }

    if(sdkHeaders.empty()) {
      LOG_WARNING(L"Windows SDK headers not found for any architecture");
    } else {
      LOG_INFO(L"Found " + std::to_wstring(sdkHeaders.size()) + L" Windows SDK header paths");
    }
  } catch(const std::exception& e) {
    LOG_ERROR(L"Exception while detecting Windows SDK headers: " + utility::decodeFromUtf8(e.what()));
  }
  return sdkHeaders;
}

std::optional<FilePath> queryVsInstallationPath(const FilePath& vsWherePath) {
  try {
    const utility::ProcessOutput out = utility::executeProcess(vsWherePath.wstr(),
                                                               {L"-latest", L"-property", L"installationPath"},
                                                               FilePath(),    // Working directory
                                                               false,         // Don't capture stderr
                                                               VSWHERE_TIMEOUT_MS);

    // Check for non-zero exit code
    if(out.exitCode != 0) {
      LOG_WARNING(L"vswhere.exe exited with code " + std::to_wstring(out.exitCode));
      return std::nullopt;
    }

    // Validate output
    if(out.output.empty()) {
      LOG_WARNING(L"vswhere.exe returned empty installation path");
      return std::nullopt;
    }

    // Trim whitespace (crucial - vswhere adds newline)
    const std::wstring trimmedOutput = utility::trim(out.output);
    if(trimmedOutput.empty()) {
      LOG_WARNING(L"vswhere.exe returned only whitespace");
      return std::nullopt;
    }

    FilePath vsInstallPath(trimmedOutput);

    // Validate the path exists
    if(!vsInstallPath.exists()) {
      LOG_WARNING(L"VS installation path does not exist: " + vsInstallPath.wstr());
      return std::nullopt;
    }

    // Basic path traversal protection - ensure it's in Program Files
    const std::wstring pathStr = vsInstallPath.wstr();
    if(pathStr.find(L"Program Files") == std::wstring::npos && pathStr.find(L"Program Files (x86)") == std::wstring::npos) {
      LOG_WARNING(L"VS installation path is not in Program Files: " + pathStr);
      // Don't return nullopt - this might be a valid custom installation
    }

    return vsInstallPath;
  } catch(const std::exception& e) {
    LOG_ERROR(L"Exception while executing vswhere.exe: " + utility::decodeFromUtf8(e.what()));
    return std::nullopt;
  }
}

void collectMsvcToolchainHeaders(const FilePath& vsInstallPath, std::vector<FilePath>& headerPaths) {
  try {
    const FilePath msvcToolsPath = vsInstallPath.getConcatenated(L"VC/Tools/MSVC");

    if(!msvcToolsPath.exists()) {
      LOG_WARNING(L"MSVC tools directory does not exist: " + msvcToolsPath.wstr());
      return;
    }

    // Get all MSVC version directories (e.g., 14.29.30133, 14.30.30705)
    const std::vector<FilePath> versionDirs = FileSystem::getDirectSubDirectories(msvcToolsPath);

    if(versionDirs.empty()) {
      LOG_WARNING(L"No MSVC toolchain versions found in: " + msvcToolsPath.wstr());
      return;
    }

    // Process each version directory
    for(const FilePath& versionPath : versionDirs) {
      // Standard include directory
      const FilePath includePath = versionPath.getConcatenated(L"include");
      if(includePath.exists()) {
        headerPaths.push_back(includePath);
        LOG_INFO(L"Added MSVC include path: " + includePath.wstr());
      } else {
        LOG_WARNING(L"MSVC include path does not exist: " + includePath.wstr());
      }

      // ATL/MFC include directory (optional)
      const FilePath atlmfcPath = versionPath.getConcatenated(L"atlmfc/include");
      if(atlmfcPath.exists()) {
        headerPaths.push_back(atlmfcPath);
        LOG_INFO(L"Added ATL/MFC include path: " + atlmfcPath.wstr());
      }
      // Don't warn if ATL/MFC is missing - it's optional
    }

  } catch(const std::exception& e) {
    LOG_ERROR(L"Exception while collecting MSVC toolchain headers: " + utility::decodeFromUtf8(e.what()));
  }
}

void collectVsAuxiliaryHeaders(const FilePath& vsInstallPath, std::vector<FilePath>& headerPaths) {
  // Array of auxiliary header paths to check
  const std::vector<std::wstring> auxiliaryPaths = {L"VC/Auxiliary/VS/include", L"VC/Auxiliary/VS/UnitTest/include"};

  for(const auto& relativePath : auxiliaryPaths) {
    try {
      const FilePath fullPath = vsInstallPath.getConcatenated(relativePath);

      if(fullPath.exists()) {
        headerPaths.push_back(fullPath);
        LOG_INFO(L"Added VS auxiliary path: " + fullPath.wstr());
      } else {
        // Don't warn for UnitTest headers as they're optional
        if(relativePath.find(L"UnitTest") == std::wstring::npos) {
          LOG_WARNING(L"VS auxiliary path does not exist: " + fullPath.wstr());
        }
      }

    } catch(const std::exception& e) {
      LOG_ERROR(L"Exception while adding auxiliary path '" + relativePath + L"': " + utility::decodeFromUtf8(e.what()));
    }
  }
}

void deduplicatePaths(std::vector<FilePath>& paths) {
  if(paths.empty()) {
    return;
  }

  // Use a set for O(n log n) deduplication
  // Note: Assumes FilePath has operator< defined
  std::set<FilePath> uniquePaths(paths.begin(), paths.end());

  if(uniquePaths.size() < paths.size()) {
    const size_t duplicatesRemoved = paths.size() - uniquePaths.size();
    LOG_INFO(L"Removed " + std::to_wstring(duplicatesRemoved) + L" duplicate paths");

    paths.assign(uniquePaths.begin(), uniquePaths.end());
  }
}
}    // namespace

CxxVs17HeaderPathDetector::CxxVs17HeaderPathDetector() : PathDetector{"Visual Studio 2017+"} {}

std::vector<FilePath> CxxVs17HeaderPathDetector::doGetPaths() const {
  std::vector<FilePath> headerSearchPaths;

  // Step 1: Find vswhere.exe
  const auto vsWherePathOpt = findVsWhereExecutable();
  if(!vsWherePathOpt) {
    LOG_WARNING(L"vswhere.exe not found - Visual Studio 2017+ may not be installed");
    // Still try to get Windows SDK headers as they're independent
    return detectWindowsSdkHeaders();
  }

  // Step 2: Query VS installation path
  const auto vsInstallPathOpt = queryVsInstallationPath(*vsWherePathOpt);
  if(!vsInstallPathOpt) {
    LOG_WARNING(L"Failed to query Visual Studio installation path");
    return detectWindowsSdkHeaders();
  }

  const FilePath& vsInstallPath = *vsInstallPathOpt;
  LOG_INFO(L"Found Visual Studio installation at: " + vsInstallPath.wstr());

  // Step 3: Collect MSVC toolchain headers (e.g., C:\...\VC\Tools\MSVC\14.xx.xxxxx\include)
  collectMsvcToolchainHeaders(vsInstallPath, headerSearchPaths);

  // Step 4: Collect VS auxiliary headers
  collectVsAuxiliaryHeaders(vsInstallPath, headerSearchPaths);

  // Step 5: Add Windows SDK headers (independent of VS installation)
  const auto sdkHeaders = detectWindowsSdkHeaders();
  utility::append(headerSearchPaths, sdkHeaders);

  // Step 6: Remove duplicates
  deduplicatePaths(headerSearchPaths);

  if(headerSearchPaths.empty()) {
    LOG_WARNING(L"No header search paths were detected");
  } else {
    LOG_INFO(L"Detected " + std::to_wstring(headerSearchPaths.size()) + L" header search paths");
  }

  return headerSearchPaths;
}
