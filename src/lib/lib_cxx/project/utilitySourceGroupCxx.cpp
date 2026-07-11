#include "utilitySourceGroupCxx.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <clang/Tooling/JSONCompilationDatabase.h>
#include <unordered_map>

#include "CanonicalFilePathCache.h"
#include "CxxCompilationDatabaseSingle.h"
#include "CxxDiagnosticConsumer.h"
#include "CxxParser.h"
#include "DialogView.h"
#include "FilePathFilter.h"
#include "FileRegister.h"
#include "FileSystem.h"
#include "GeneratePCHAction.h"
#include "logging.h"
#include "ParserClientImpl.h"
#include "SingleFrontendActionFactory.h"
#include "SourceGroupSettingsWithCxxPchOptions.h"
#include "StorageProvider.h"
#include "TaskLambda.h"
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
    dialogView->showUnknownProgressDialog(L"Preparing Indexing", L"Processing Precompiled Headers");
    // NOLINTNEXTLINE(bugprone-lambda-function-name): It will be solved with SOUR-125
    LOG_INFO(L"Generating precompiled header output for input file \"{}\" at location \"{}\"",
             pchInputFilePath.wstr(),
             pchOutputFilePath.wstr());

    CxxParser::initializeLLVM();

    if(!pchOutputFilePath.getParentDirectory().exists()) {
      FileSystem::createDirectory(pchOutputFilePath.getParentDirectory());
    }

    const std::shared_ptr<IntermediateStorage> storage = std::make_shared<IntermediateStorage>();
    const std::shared_ptr<ParserClientImpl> client = std::make_shared<ParserClientImpl>(storage.get());

    const std::shared_ptr<FileRegister> fileRegister = std::make_shared<FileRegister>(
        pchInputFilePath, std::set<FilePath>{pchInputFilePath}, std::set<FilePathFilter>{});

    const std::shared_ptr<CanonicalFilePathCache> canonicalFilePathCache = std::make_shared<CanonicalFilePathCache>(fileRegister);

    clang::tooling::CompileCommand pchCommand;
    pchCommand.Filename = utility::encodeToUtf8(pchInputFilePath.fileName());
    pchCommand.Directory = pchOutputFilePath.getParentDirectory().str();
    // DON'T use "-fsyntax-only" here because it will cause the output file to be erased
    pchCommand.CommandLine = utility::concat({"clang-tool"}, CxxParser::getCommandlineArgumentsEssential(compilerFlags));

    const CxxCompilationDatabaseSingle compilationDatabase(pchCommand);
    clang::tooling::ClangTool tool(compilationDatabase, {utility::encodeToUtf8(pchInputFilePath.wstr())});
    auto* action = new GeneratePCHAction(client, canonicalFilePathCache);    // NOLINT(cppcoreguidelines-owning-memory)

    const llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> options =
        new clang::DiagnosticOptions;    // NOLINT(cppcoreguidelines-owning-memory)
    CxxDiagnosticConsumer diagnostics(llvm::errs(), &*options, client, canonicalFilePathCache, pchInputFilePath, true);

    tool.setDiagnosticConsumer(&diagnostics);
    tool.clearArgumentsAdjusters();
    tool.run(new SingleFrontendActionFactory(action));    // NOLINT(cppcoreguidelines-owning-memory)

    storageProvider->insert(storage);
  });
}

std::shared_ptr<clang::tooling::JSONCompilationDatabase> loadCDB(const FilePath& cdbPath, std::string* error) {
  if(cdbPath.empty() || !cdbPath.exists()) {
    return {};
  }

  std::string errorString;
  std::shared_ptr<clang::tooling::JSONCompilationDatabase> cdb = std::shared_ptr<clang::tooling::JSONCompilationDatabase>(
      clang::tooling::JSONCompilationDatabase::loadFromFile(
          utility::encodeToUtf8(cdbPath.wstr()), errorString, clang::tooling::JSONCommandLineSyntax::AutoDetect));

  if((error != nullptr) && !errorString.empty()) {
    *error = errorString;
  }

  return cdb;
}

bool containsIncludePchFlags(const std::shared_ptr<clang::tooling::JSONCompilationDatabase>& cdb) {
  for(const clang::tooling::CompileCommand& command : cdb->getAllCompileCommands()) {
    if(containsIncludePchFlag(command.CommandLine)) {
      return true;
    }
  }
  return false;
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
