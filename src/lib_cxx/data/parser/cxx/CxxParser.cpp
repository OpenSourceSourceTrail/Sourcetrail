#include "CxxParser.h"
// clang
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Options.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Tooling/Tooling.h>
// llvm
#include <utility>

#include <llvm/Option/ArgList.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/VirtualFileSystem.h>
// internal
#include "ASTAction.h"
#include "CanonicalFilePathCache.h"
#include "ClangInvocationInfo.h"
#include "CxxCompilationDatabaseSingle.h"
#include "CxxDiagnosticConsumer.h"
#include "FilePath.h"
#include "FileRegister.h"
#include "IndexerCommandCxx.h"
#include "ParserClient.h"
#include "SingleFrontendActionFactory.h"
#include "TextAccess.h"
#include "IApplicationSettings.hpp"
#include "logging.h"
#include "utility.h"
#include "utilityString.h"

namespace {
std::vector<std::string> prependSyntaxOnlyToolArgs(const std::vector<std::string>& args) {
  return utility::concat(std::vector<std::string>({"clang-tool", "-fsyntax-only"}), args);
}

std::vector<std::string> appendFilePath(const std::vector<std::string>& args, llvm::StringRef filePath) {
  return utility::concat(args, {filePath.str()});
}

// custom implementation of clang::runToolOnCodeWithArgs which also sets our custom DiagnosticConsumer
bool runToolOnCodeWithArgs(
    clang::DiagnosticConsumer* DiagConsumer,
    std::unique_ptr<clang::FrontendAction> ToolAction,
    const llvm::Twine& Code,
    const std::vector<std::string>& Args,
    const llvm::Twine& FileName = "input.cc",
    [[maybe_unused]] const clang::tooling::FileContentMappings& VirtualMappedFiles = clang::tooling::FileContentMappings()) {
  CxxParser::initializeLLVM();

  llvm::SmallString<16> FileNameStorage;
  llvm::StringRef FileNameRef = FileName.toNullTerminatedStringRef(FileNameStorage);

  llvm::IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> OverlayFileSystem(
      new llvm::vfs::OverlayFileSystem(llvm::vfs::getRealFileSystem()));
  llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> InMemoryFileSystem(new llvm::vfs::InMemoryFileSystem);
  OverlayFileSystem->pushOverlay(InMemoryFileSystem);
  llvm::IntrusiveRefCntPtr<clang::FileManager> Files(new clang::FileManager(clang::FileSystemOptions(), OverlayFileSystem));

  clang::tooling::ToolInvocation Invocation(
      prependSyntaxOnlyToolArgs(appendFilePath(Args, FileNameRef)), std::move(ToolAction), Files.get());

  llvm::SmallString<1024> CodeStorage;
  llvm::StringRef CodeRef = Code.toNullTerminatedStringRef(CodeStorage);

  InMemoryFileSystem->addFile(FileNameRef, 0, llvm::MemoryBuffer::getMemBufferCopy(CodeRef));

  Invocation.setDiagnosticConsumer(DiagConsumer);

  return Invocation.run();
}
}    // namespace

std::vector<std::string> CxxParser::getCommandlineArgumentsEssential(const std::vector<std::wstring>& compilerFlags) {
  std::vector<std::string> args;

  // The option -fno-delayed-template-parsing signals that templates that there should
  // be AST elements for unused template functions as well.
  args.emplace_back("-fno-delayed-template-parsing");

  // The option -fexceptions signals that clang should watch out for exception-related code during
  // indexing.
  args.emplace_back("-fexceptions");

  // The option -c signals that no executable is built.
  args.emplace_back("-c");

  // The option -w disables all warnings.
  args.emplace_back("-w");

  // This option tells clang just to continue parsing no matter how manny errors have been thrown.
  args.emplace_back("-ferror-limit=0");

  for(const auto& compilerFlag : compilerFlags) {
    args.push_back(utility::encodeToUtf8(compilerFlag));
  }

  return args;
}

void CxxParser::initializeLLVM() {
  static bool initialized = false;
  if(!initialized) {
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();
    initialized = true;
  }
}

CxxParser::CxxParser(std::shared_ptr<ParserClient> client,
                     std::shared_ptr<FileRegister> fileRegister,
                     std::shared_ptr<IndexerStateInfo> indexerStateInfo)
    : Parser(std::move(client)), m_fileRegister(std::move(fileRegister)), m_indexerStateInfo(std::move(indexerStateInfo)) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
}

void CxxParser::buildIndex(const std::shared_ptr<IndexerCommandCxx>& indexerCommand) {
  clang::tooling::CompileCommand compileCommand;
  compileCommand.Filename = utility::encodeToUtf8(indexerCommand->getSourceFilePath().wstr());
  compileCommand.Directory = utility::encodeToUtf8(indexerCommand->getWorkingDirectory().wstr());
  auto args = indexerCommand->getCompilerFlags();
  if(!args.empty() && !utility::isPrefix<std::wstring>(L"-", args.front())) {
    args.erase(args.begin());
  }
  compileCommand.CommandLine = getCommandlineArgumentsEssential(args);
  compileCommand.CommandLine = prependSyntaxOnlyToolArgs(compileCommand.CommandLine);

  CxxCompilationDatabaseSingle compilationDatabase(compileCommand);
  runTool(&compilationDatabase, indexerCommand->getSourceFilePath());
}

void CxxParser::buildIndex(const std::wstring& fileName,
                           const std::shared_ptr<TextAccess>& fileContent,
                           const std::vector<std::wstring>& compilerFlags) {
  auto canonicalFilePathCache = std::make_shared<CanonicalFilePathCache>(m_fileRegister);

  auto diagnostics = getDiagnostics(FilePath(), canonicalFilePathCache, false);
  auto action = std::make_unique<ASTAction>(m_client, canonicalFilePathCache, m_indexerStateInfo);

  auto args = getCommandlineArgumentsEssential(compilerFlags);

  runToolOnCodeWithArgs(diagnostics.get(), std::move(action), fileContent->getText(), args, utility::encodeToUtf8(fileName));
}

void CxxParser::runTool(clang::tooling::CompilationDatabase* pCompilationDatabase, const FilePath& sourceFilePath) {
  initializeLLVM();

  clang::tooling::ClangTool tool(*pCompilationDatabase, std::vector<std::string>(1, utility::encodeToUtf8(sourceFilePath.wstr())));

  auto pCanonicalFilePathCache = std::make_shared<CanonicalFilePathCache>(m_fileRegister);
  auto pDiagnostics = getDiagnostics(sourceFilePath, pCanonicalFilePathCache, true);

  tool.setDiagnosticConsumer(pDiagnostics.get());

  ClangInvocationInfo info;
  info = ClangInvocationInfo::getClangInvocationString(pCompilationDatabase);
  LOG_INFO("Clang Invocation: " +
            info.invocation.substr(
                0, IApplicationSettings::getInstanceRaw()->getVerboseIndexerLoggingEnabled() ? std::string::npos : 20000));

  if(!info.errors.empty()) {
    LOG_INFO("Clang Invocation errors: " + info.errors);
  }

  auto* pAction = new ASTAction(m_client, pCanonicalFilePathCache, m_indexerStateInfo);
  tool.run(new SingleFrontendActionFactory(pAction));

  if(!m_client->hasContent()) {
    if(info.invocation.empty()) {
      info = ClangInvocationInfo::getClangInvocationString(pCompilationDatabase);
    }

    if(!info.errors.empty()) {
      Id fileId = m_client->recordFile(sourceFilePath, true);
      m_client->recordError(L"Clang Invocation errors: " + utility::decodeFromUtf8(info.errors),
                            true,
                            true,
                            sourceFilePath,
                            ParseLocation(fileId, 1, 1));
    }
  }
}

std::shared_ptr<CxxDiagnosticConsumer> CxxParser::getDiagnostics(const FilePath& sourceFilePath,
                                                                 std::shared_ptr<CanonicalFilePathCache> canonicalFilePathCache,
                                                                 bool logErrors) const {
  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> options = new clang::DiagnosticOptions();
  return std::make_shared<CxxDiagnosticConsumer>(
      llvm::errs(), &*options, m_client, std::move(canonicalFilePathCache), sourceFilePath, logErrors);
}