#pragma once
#include <filesystem>
#include <string>

#include "ApplicationArchitectureType.h"
#include "FilePath.h"
#include "OsType.h"

namespace utility {

struct ProcessOutput final {
  std::wstring output;
  std::wstring error;
  int exitCode;
};

std::filesystem::path searchPath(const std::filesystem::path& bin, bool& ok);

std::filesystem::path searchPath(const std::filesystem::path& bin);

ProcessOutput executeProcess(const std::wstring& command,
                             const std::vector<std::wstring>& arguments,
                             const FilePath& workingDirectory = {},
                             const bool waitUntilNoOutput = false,
                             const int timeout = 30000,
                             bool logProcessOutput = false);

void killRunningProcesses();

int getIdealThreadCount();

constexpr OsType getOsType() {
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
  return OsType::Windows;
#elif defined(__APPLE__)
  return OsType::Mac;
#elif defined(__linux) || defined(__linux__) || defined(linux)
  return OsType::Linux;
#else
  return OsType::Unkown;
#endif
}

constexpr ApplicationArchitectureType getApplicationArchitectureType() {
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64) || defined(WIN64)
  return ApplicationArchitectureType::X86_64;
#else
  return ApplicationArchitectureType::X86_32;
#endif
  return ApplicationArchitectureType::Unknown;
}

std::string getAppArchTypeString();

}    // namespace utility
