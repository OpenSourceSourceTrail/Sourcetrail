#include "utilitySourceGroupCxx.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/xchar.h>

#include "DialogView.h"
#include "FilePath.h"
#include "ICxxToolchain.h"
#include "IntermediateStorage.h"
#include "logging.h"
#include "OrderedCache.h"
#include "SourceGroupSettingsWithCxxPchOptions.h"
#include "StorageProvider.h"
#include "TaskLambda.h"
#include "type/MessageStatus.h"
#include "utility.h"
#include "utilityString.h"

namespace {
bool contains(const std::wstring& text, const std::wstring& value) {
  return text.find(value) != std::wstring::npos;
}
}    // namespace

namespace utility {
std::shared_ptr<Task> createBuildPchTask(const SourceGroupSettingsWithCxxPchOptions* settings,
                                         std::vector<std::wstring> compilerFlags,
                                         const std::shared_ptr<StorageProvider>& storageProvider,
                                         const std::shared_ptr<DialogView>& dialogView) {
  const FilePath pchInputFilePath = settings->getPchInputFilePathExpandedAndAbsolute();
  const FilePath pchDependenciesDirectoryPath = settings->getPchDependenciesDirectoryPath();

  if(pchInputFilePath.empty() || pchDependenciesDirectoryPath.empty()) {
    return std::make_shared<TaskLambda>([]() {});
  }

  if(!pchInputFilePath.exists()) {
    LOG_ERROR(L"Precompiled header input file \"{}\" does not exist.", pchInputFilePath.wstr());
    return std::make_shared<TaskLambda>([]() {});
  }

  const FilePath pchOutputFilePath =
      pchDependenciesDirectoryPath.getConcatenated(pchInputFilePath.fileName()).replaceExtension(L"pch");

  utility::removeIncludePchFlag(compilerFlags);
  compilerFlags.push_back(pchInputFilePath.wstr());
  compilerFlags.emplace_back(L"-emit-pch");
  compilerFlags.emplace_back(L"-o");
  compilerFlags.push_back(pchOutputFilePath.wstr());

  return std::make_shared<TaskLambda>([dialogView, storageProvider, pchInputFilePath, pchOutputFilePath, compilerFlags]() {
    const ICxxToolchain* toolchain = ICxxToolchain::getInstance();
    if(toolchain == nullptr) {
      // NOLINTNEXTLINE(bugprone-lambda-function-name): It will be solved with SOUR-125
      LOG_WARNING(L"Skipping the precompiled header for \"{}\": this process has no C/C++ toolchain.", pchInputFilePath.wstr());
      return;
    }

    dialogView->showUnknownProgressDialog(L"Preparing Indexing", L"Processing Precompiled Headers");
    // NOLINTNEXTLINE(bugprone-lambda-function-name): It will be solved with SOUR-125
    LOG_INFO(L"Generating precompiled header output for input file \"{}\" at location \"{}\"",
             pchInputFilePath.wstr(),
             pchOutputFilePath.wstr());

    if(const std::shared_ptr<IntermediateStorage> storage = toolchain->buildPrecompiledHeader(
           pchInputFilePath, pchOutputFilePath, compilerFlags)) {
      storageProvider->insert(storage);
    }
  });
}

std::optional<std::vector<CxxCompileCommand>> loadCompilationDatabase(const FilePath& cdbPath, std::string* error) {
  const ICxxToolchain* toolchain = ICxxToolchain::getInstance();
  if(toolchain == nullptr) {
    if(error != nullptr) {
      *error = "This process was built without the C/C++ language package.";
    }
    return std::nullopt;
  }
  return toolchain->loadCompilationDatabase(cdbPath, error);
}

std::vector<FilePath> getSourceFilesFromCDB(const FilePath& cdbPath) {
  std::string error;
  const std::optional<std::vector<CxxCompileCommand>> commands = loadCompilationDatabase(cdbPath, &error);

  if(!error.empty()) {
    const auto message = fmt::format(L"Loading Clang compilation database failed with error: \"{}\"",
                                     utility::decodeFromUtf8(error));
    LOG_ERROR(message);
    MessageStatus(message, true).dispatch();
  }

  return commands ? getSourceFilesFromCDB(*commands, cdbPath) : std::vector<FilePath>{};
}

std::vector<FilePath> getSourceFilesFromCDB(const std::vector<CxxCompileCommand>& commands, const FilePath& cdbPath) {
  OrderedCache<FilePath, FilePath> canonicalDirectoryPathCache([](const FilePath& path) { return path.getCanonical(); });

  std::vector<FilePath> filePaths;
  filePaths.reserve(commands.size());
  for(const CxxCompileCommand& command : commands) {
    FilePath path = FilePath(utility::decodeFromUtf8(command.file));
    if(!path.isAbsolute()) {
      path = FilePath(utility::decodeFromUtf8(command.directory + '/' + command.file)).makeCanonical();
    }
    if(!path.isAbsolute()) {
      path = cdbPath.getParentDirectory().getConcatenated(path).makeCanonical();
    }
    filePaths.push_back(canonicalDirectoryPathCache.getValue(path.getParentDirectory()).concatenate(path.fileName()));
  }
  return filePaths;
}

bool containsIncludePchFlags(const std::vector<CxxCompileCommand>& commands) {
  return std::ranges::any_of(commands, [](const CxxCompileCommand& command) { return containsIncludePchFlag(command.arguments); });
}

bool containsIncludePchFlag(const std::vector<std::string>& args) {
  const std::string includePchPrefix = "-include-pch";
  for(const auto& item : args) {
    const std::string arg = utility::trim(item);
    if(utility::isPrefix(includePchPrefix, arg)) {
      return true;
    }
  }
  return false;
}

std::vector<std::wstring> getWithRemoveIncludePchFlag(const std::vector<std::wstring>& args) {
  std::vector<std::wstring> ret = args;
  removeIncludePchFlag(ret);
  return ret;
}

bool convertWindowsStyleFlagsToUnixStyleFlags(std::vector<std::wstring>& args) {
  static const std::unordered_map<std::wstring, std::wstring> windowsToUnix = {
      {L"/std", L"-std"},
      {L"/GR", L"-frtti"},
      {L"/GR-", L"-fno-rtti"},
      {L"/I", L"-I"},
      {L"/D", L"-D"},
      {L"/EHsc", L"-fexceptions"},
      {L"/EHsc-", L"-fno-exceptions"},
      {L"-external:I", L"-isystem"},
      {L"-std:", L"-std="},
  };
  static constexpr std::array ValidUnix = {L"-std=", L"-D", L"-I", L"-isystem"};

  if(args.empty()) {
    return false;
  }

  std::vector<std::wstring> output;
  output.reserve(args.size());

  auto itr = args.cbegin();
  if(contains(args.front(), L"cl.exe")) {
    output.push_back(*itr);
    std::advance(itr, 1);
  }

  for(; itr != args.cend(); std::advance(itr, 1)) {
    const auto& value = *itr;
    if(value.size() < 2) {
      continue;
    }

    if('-' == value[0]) {
      if(std::ranges::any_of(ValidUnix, [&value](const auto& item) { return contains(value, item); })) {
        output.push_back(*itr);
        continue;
      } else {
        std::wstring key = L"-external:I";
        std::wstring unixValue = L"-isystem";

        if(contains(value, key)) {
          output.push_back(fmt::format(L"{}{}", unixValue, value.substr(key.size())));
          continue;
        }

        key = L"-std:";
        unixValue = L"-std=";

        if(contains(value, key)) {
          output.push_back(fmt::format(L"{}{}", unixValue, value.substr(key.size())));
          continue;
        }

        if(contains(value, L"-c") && itr + 1 != args.cend()) {
          output.emplace_back(L"-c");
          output.emplace_back(*(itr + 1));
        }
      }
    }

    if('/' == value[0]) {
      for(const auto& key : windowsToUnix) {
        if(contains(value, key.first)) {
          output.push_back(fmt::format(L"{}{}", key.second, value.substr(key.first.size())));
          continue;
        }
      }
    }
  }

  args = std::move(output);
  return true;
}

void removeIncludePchFlag(std::vector<std::wstring>& args) {
  const std::wstring includePchPrefix = L"-include-pch";
  for(size_t index = 0; index < args.size(); index++) {
    const std::wstring arg = utility::trim(args[index]);
    if(utility::isPrefix<std::wstring>(includePchPrefix, arg)) {
      if(index + 1 < args.size() && !utility::isPrefix<std::wstring>(L"-", utility::trim(args[index + 1])) &&
         arg == includePchPrefix) {
        args.erase(args.begin() + static_cast<long>(index) + 1);
      }
      args.erase(args.begin() + static_cast<long>(index));
      index--;
    }
  }
}

std::vector<std::wstring> getIncludePchFlags(const SourceGroupSettingsWithCxxPchOptions* settings) {
  const FilePath pchInputFilePath = settings->getPchInputFilePathExpandedAndAbsolute();
  const FilePath pchDependenciesDirectoryPath = settings->getPchDependenciesDirectoryPath();

  if(!pchInputFilePath.empty() && !pchDependenciesDirectoryPath.empty()) {
    const FilePath pchOutputFilePath =
        pchDependenciesDirectoryPath.getConcatenated(pchInputFilePath.fileName()).replaceExtension(L"pch");

    return {L"-fallow-pch-with-compiler-errors", L"-include-pch", pchOutputFilePath.wstr()};
  }

  return {};
}
}    // namespace utility
