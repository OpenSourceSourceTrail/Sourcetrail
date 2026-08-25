#include "CxxToolchainLocal.h"

#include <set>

#include <clang/Tooling/JSONCompilationDatabase.h>
#include <clang/Tooling/Tooling.h>

#include "CanonicalFilePathCache.h"
#include "CxxCompilationDatabaseSingle.h"
#include "CxxDiagnosticConsumer.h"
#include "CxxParser.h"
#include "FilePathFilter.h"
#include "FileRegister.h"
#include "FileSystem.h"
#include "GeneratePCHAction.h"
#include "IntermediateStorage.h"
#include "ParserClientImpl.h"
#include "SingleFrontendActionFactory.h"
#include "utility.h"
#include "utilityString.h"

std::optional<std::vector<CxxCompileCommand>> CxxToolchainLocal::loadCompilationDatabase(const FilePath& cdbPath,
                                                                                         std::string* error) const {
  if(cdbPath.empty() || !cdbPath.exists()) {
    return std::nullopt;
  }

  std::string errorString;
  const std::unique_ptr<clang::tooling::JSONCompilationDatabase> cdb = clang::tooling::JSONCompilationDatabase::loadFromFile(
      utility::encodeToUtf8(cdbPath.wstr()), errorString, clang::tooling::JSONCommandLineSyntax::AutoDetect);

  if((error != nullptr) && !errorString.empty()) {
    *error = errorString;
  }

  if(!cdb) {
    return std::nullopt;
  }

  std::vector<CxxCompileCommand> commands;
  for(const clang::tooling::CompileCommand& command : cdb->getAllCompileCommands()) {
    commands.emplace_back(CxxCompileCommand{command.Directory, command.Filename, command.CommandLine});
  }
  return commands;
}

std::shared_ptr<IntermediateStorage> CxxToolchainLocal::buildPrecompiledHeader(
    const FilePath& pchInputFilePath, const FilePath& pchOutputFilePath, const std::vector<std::wstring>& compilerFlags) const {
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

  CxxDiagnosticConsumer diagnostics(client, canonicalFilePathCache, pchInputFilePath);

  tool.setDiagnosticConsumer(&diagnostics);
  // clang::tooling::ClangTool otherwise writes its own failure messages straight to stderr
  tool.setPrintErrorMessage(false);
  tool.clearArgumentsAdjusters();
  tool.run(new SingleFrontendActionFactory(action));    // NOLINT(cppcoreguidelines-owning-memory)

  return storage;
}
