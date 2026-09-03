#include "project/logic/utilitySourceGroupCxx.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/xchar.h>

#include <unordered_map>

#include "component/view/DialogView.h"
#include "data/storage/IntermediateStorage.h"
#include "data/storage/StorageProvider.h"
#include "FilePath.h"
#include "FilePathFilter.h"
#include "FileSystem.h"
#include "logging.h"
#include "OrderedCache.h"
#include "project/logic/ICxxToolchain.h"
#include "settings/source_group/component/cxx/SourceGroupSettingsWithCxxPchOptions.h"
#include "status/messages/MessageStatus.h"
#include "TaskLambda.h"
#include "TextAccess.h"
#include "utility.h"
#include "utilityFile.h"
#include "utilityString.h"

namespace {
bool contains(const std::wstring& text, const std::wstring& value) {
  return text.find(value) != std::wstring::npos;
}

// An automatic precompiled header only pays off once several files share their includes, and the
// prefix has to stay small enough that the parses which do not need every header are not the ones
// paying for it.
constexpr size_t MinSourceFilesForAutoPch = 4;
constexpr size_t MinFilesSharingInclude = 3;
constexpr double MinShareOfFiles = 0.25;
constexpr size_t MaxAutoPchIncludes = 64;
// Include blocks live at the top of a file. Reading further costs a lot on generated sources and
// finds nothing a prefix header may hold.
constexpr uint32_t MaxScannedLines = 400;

/** The `<...>` of an `#include <...>` line, empty for every other line. */
std::string angleIncludeOf(const std::string& line) {
  const std::string trimmed = utility::trim(line);
  if(trimmed.empty() || trimmed.front() != '#') {
    return {};
  }
  const std::string directive = utility::trim(trimmed.substr(1));
  if(!utility::isPrefix<std::string>("include", directive)) {
    return {};
  }
  const std::string argument = utility::trim(directive.substr(std::string("include").size()));
  if(argument.empty() || argument.front() != '<') {
    return {};
  }
  const size_t end = argument.find('>');
  if(end == std::string::npos) {
    return {};
  }
  return utility::trim(argument.substr(1, end - 1));
}

bool isMacroDefinition(const std::string& line) {
  const std::string trimmed = utility::trim(line);
  if(trimmed.empty() || trimmed.front() != '#') {
    return false;
  }
  return utility::isPrefix<std::string>("define", utility::trim(trimmed.substr(1)));
}

/** The -I and -isystem directories a command line names, in both their spellings. */
std::vector<FilePath> includeDirsOf(const std::vector<std::wstring>& compilerFlags) {
  static const std::array<std::wstring, 2> Prefixes = {L"-I", L"-isystem"};

  std::vector<FilePath> dirs;
  for(size_t index = 0; index < compilerFlags.size(); index++) {
    const std::wstring flag = utility::trim(compilerFlags[index]);
    for(const std::wstring& prefix : Prefixes) {
      if(!utility::isPrefix(prefix, flag)) {
        continue;
      }
      const std::wstring value = (flag == prefix) ? (index + 1 < compilerFlags.size() ? compilerFlags[index + 1] : L"") :
                                                    flag.substr(prefix.size());
      if(!value.empty()) {
        dirs.emplace_back(utility::trim(value));
      }
      break;
    }
  }
  return dirs;
}

/**
 * A command line reduced to the flags a precompiled header build can reuse.
 *
 * A compilation database entry names the compiler, the source file and an object file. None of them
 * belong in a build whose input is the generated prefix header and whose output is the .pch.
 */
std::vector<std::wstring> flagsWithoutInputsAndOutputs(const std::vector<std::wstring>& compilerFlags) {
  static const std::array<std::wstring, 14> FlagsTakingAValue = {L"-Xclang",
                                                                 L"-I",
                                                                 L"-isystem",
                                                                 L"-iquote",
                                                                 L"-idirafter",
                                                                 L"-imacros",
                                                                 L"-include",
                                                                 L"-isysroot",
                                                                 L"--sysroot",
                                                                 L"-F",
                                                                 L"-D",
                                                                 L"-U",
                                                                 L"-x",
                                                                 L"-target"};

  std::vector<std::wstring> flags;
  for(size_t index = 0; index < compilerFlags.size(); index++) {
    const std::wstring flag = utility::trim(compilerFlags[index]);
    if(flag == L"-c") {
      continue;
    }
    if(flag == L"-o") {
      index++;
      continue;
    }
    if(!utility::isPrefix<std::wstring>(L"-", flag)) {
      // The compiler and the files it was told to compile.
      continue;
    }

    flags.push_back(compilerFlags[index]);
    if(std::ranges::find(FlagsTakingAValue, flag) != FlagsTakingAValue.end() && index + 1 < compilerFlags.size()) {
      flags.push_back(compilerFlags[++index]);
    }
  }
  return flags;
}

/** The `-x` language a prefix header has to be compiled as for this command line. */
std::wstring headerLanguageOf(const std::vector<std::wstring>& compilerFlags) {
  for(const std::wstring& flag : compilerFlags) {
    if(utility::isPrefix<std::wstring>(L"-std=c++", utility::trim(flag)) || utility::trim(flag) == L"c++") {
      return L"c++-header";
    }
  }
  for(const std::wstring& flag : compilerFlags) {
    if(utility::isPrefix<std::wstring>(L"-std=c", utility::trim(flag)) || utility::trim(flag) == L"c") {
      return L"c-header";
    }
  }
  return L"c++-header";
}

/**
 * The flags that make a parse read a precompiled header.
 *
 * `-fallow-pch-with-compiler-errors` is a cc1 flag, so it only reaches the frontend behind
 * `-Xclang`. Spelled without it the driver rejects the whole command line -- and with it every
 * parse -- rather than just the flag.
 */
std::vector<std::wstring> includePchFlagsFor(const FilePath& pchPath) {
  return {L"-Xclang", L"-fallow-pch-with-compiler-errors", L"-include-pch", pchPath.wstr()};
}

/**
 * The command line that turns one header into a .pch.
 *
 * The action doing the writing is handed to the tool directly, so there is no `-emit-pch`: the
 * driver has no such flag and rejects the whole invocation over it. What the flags still have to
 * say is the language -- a prefix header is a .h, which the driver would compile as C -- and where
 * the output goes.
 */
std::vector<std::wstring> pchBuildFlags(const std::vector<std::wstring>& compilerFlags,
                                        const FilePath& headerPath,
                                        const FilePath& pchPath) {
  std::vector<std::wstring> flags = flagsWithoutInputsAndOutputs(utility::getWithRemoveIncludePchFlag(compilerFlags));
  flags.emplace_back(L"-x");
  flags.push_back(headerLanguageOf(compilerFlags));
  flags.push_back(headerPath.wstr());
  flags.emplace_back(L"-o");
  flags.push_back(pchPath.wstr());
  return flags;
}

/** Whether an include reaches a header the project indexes itself. */
bool resolvesIntoIndexedPaths(const std::string& include,
                              const std::vector<FilePath>& includeDirs,
                              const std::set<FilePath>& indexedPaths) {
  for(const FilePath& includeDir : includeDirs) {
    const FilePath candidate = includeDir.getConcatenated(utility::decodeFromUtf8(include));
    if(!candidate.exists()) {
      continue;
    }
    for(const FilePath& indexedPath : indexedPaths) {
      if(indexedPath == candidate || indexedPath.contains(candidate)) {
        return true;
      }
    }
  }
  return false;
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

  compilerFlags = pchBuildFlags(compilerFlags, pchInputFilePath, pchOutputFilePath);

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
    const auto message = fmt::format(
        L"Loading Clang compilation database failed with error: \"{}\"", utility::decodeFromUtf8(error));
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

std::set<FilePath> filterCdbSourceFiles(const std::vector<FilePath>& sourceFiles,
                                        const std::vector<FilePathFilter>& excludeFilters) {
  std::set<FilePath> filtered;
  for(const FilePath& path : sourceFiles) {
    if(!FilePathFilter::areMatching(excludeFilters, path) && path.exists()) {
      filtered.insert(path);
    }
  }
  return filtered;
}

std::vector<FilePath> deriveCdbHeaderRoots(const std::vector<FilePath>& sourceFiles, const std::vector<FilePath>& headerPaths) {
  // Deduplicating the directories before canonicalizing them: a database lists thousands of files
  // across a handful of directories, and each canonicalization is a syscall.
  std::set<FilePath> directories;
  for(const FilePath& path : sourceFiles) {
    directories.insert(path.getParentDirectory());
  }

  std::set<FilePath> roots;
  for(const FilePath& path : directories) {
    roots.insert(path.getCanonical());
  }
  for(const FilePath& path : headerPaths) {
    if(path.exists()) {
      roots.insert(path.getCanonical());
    }
  }

  std::vector<FilePath> topLevel;
  for(const FilePath& path : utility::getTopLevelPaths(roots)) {
    if(path.exists()) {
      topLevel.push_back(path);
    }
  }
  return topLevel;
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

std::vector<std::string> collectAutoPchIncludes(const std::vector<FilePath>& sourceFiles,
                                                const std::vector<FilePath>& includeDirs,
                                                const std::set<FilePath>& indexedPaths) {
  if(sourceFiles.size() < MinSourceFilesForAutoPch) {
    return {};
  }

  std::unordered_map<std::string, size_t> sharingFiles;
  // A header included after a #define may expand differently for it -- <windows.h> behind
  // WIN32_LEAN_AND_MEAN is the usual one. A prefix header is included before anything the source
  // says, so such a header cannot move into one.
  std::set<std::string> macroSensitive;

  for(const FilePath& sourceFile : sourceFiles) {
    const std::shared_ptr<TextAccess> text = TextAccess::createFromFile(sourceFile);
    if(!text) {
      continue;
    }

    std::set<std::string> seen;
    bool sawMacroDefinition = false;
    const uint32_t lineCount = std::min(text->getLineCount(), MaxScannedLines);
    for(uint32_t line = 1; line <= lineCount; line++) {
      const std::string content = text->getLine(line);
      if(isMacroDefinition(content)) {
        sawMacroDefinition = true;
        continue;
      }
      const std::string include = angleIncludeOf(content);
      if(include.empty()) {
        continue;
      }
      if(sawMacroDefinition) {
        macroSensitive.insert(include);
      }
      seen.insert(include);
    }

    for(const std::string& include : seen) {
      sharingFiles[include]++;
    }
  }

  const auto threshold = std::max<size_t>(
      MinFilesSharingInclude, static_cast<size_t>(MinShareOfFiles * static_cast<double>(sourceFiles.size())));

  std::vector<std::pair<std::string, size_t>> kept;
  for(const auto& [include, files] : sharingFiles) {
    if(files >= threshold && !macroSensitive.contains(include) && !resolvesIntoIndexedPaths(include, includeDirs, indexedPaths)) {
      kept.emplace_back(include, files);
    }
  }

  std::ranges::sort(kept, [](const auto& lhs, const auto& rhs) {
    return lhs.second != rhs.second ? lhs.second > rhs.second : lhs.first < rhs.first;
  });
  if(kept.size() > MaxAutoPchIncludes) {
    kept.resize(MaxAutoPchIncludes);
  }

  std::vector<std::string> includes;
  includes.reserve(kept.size());
  for(const auto& [include, files] : kept) {
    includes.push_back(include);
  }
  return includes;
}

FilePath writeAutoPchHeader(const std::vector<std::string>& includes,
                            const FilePath& outputDirectory,
                            const std::wstring& headerName) {
  if(includes.empty()) {
    return {};
  }

  if(!outputDirectory.exists()) {
    FileSystem::createDirectory(outputDirectory);
  }

  const FilePath headerPath = outputDirectory.getConcatenated(headerName);
  std::ofstream header(headerPath.str(), std::ios::binary | std::ios::trunc);
  if(!header) {
    LOG_ERROR(L"Could not write the automatic precompiled header to \"{}\".", headerPath.wstr());
    return {};
  }
  for(const std::string& include : includes) {
    header << "#include <" << include << ">\n";
  }
  return headerPath;
}

std::wstring macroSignatureOf(const std::vector<std::wstring>& compilerFlags) {
  static const std::array<std::wstring, 4> Prefixes = {L"-D", L"-U", L"-std", L"-x"};

  std::vector<std::wstring> relevant;
  for(size_t index = 0; index < compilerFlags.size(); index++) {
    const std::wstring flag = utility::trim(compilerFlags[index]);
    for(const std::wstring& prefix : Prefixes) {
      if(!utility::isPrefix(prefix, flag)) {
        continue;
      }
      if(flag == prefix && index + 1 < compilerFlags.size()) {
        relevant.push_back(flag + L"=" + utility::trim(compilerFlags[index + 1]));
      } else {
        relevant.push_back(flag);
      }
      break;
    }
  }

  std::ranges::sort(relevant);
  return utility::join(relevant, L" ");
}

std::vector<std::wstring> buildAutoPch(const std::vector<FilePath>& sourceFiles,
                                       const std::vector<std::wstring>& compilerFlags,
                                       const std::set<FilePath>& indexedPaths,
                                       const FilePath& outputDirectory,
                                       const std::wstring& name) {
  const ICxxToolchain* toolchain = ICxxToolchain::getInstance();
  if(toolchain == nullptr || outputDirectory.empty()) {
    return {};
  }

  const std::vector<std::string> includes = collectAutoPchIncludes(sourceFiles, includeDirsOf(compilerFlags), indexedPaths);
  const FilePath headerPath = writeAutoPchHeader(includes, outputDirectory, name + L".h");
  if(headerPath.empty()) {
    return {};
  }

  const FilePath pchPath = outputDirectory.getConcatenated(name + L".pch");
  const std::vector<std::wstring> buildFlags = pchBuildFlags(compilerFlags, headerPath, pchPath);

  LOG_INFO(L"Precompiling {} shared headers for {} source files into \"{}\".", includes.size(), sourceFiles.size(), pchPath.wstr());

  // The storage the build reports describes the synthetic prefix header and the external headers it
  // pulls in. None of that is part of the project, so it is dropped rather than injected -- but its
  // errors are the only signal that the build went wrong: the generator writes its output file even
  // when the parse failed, and Clang then rejects that file for every source that includes it.
  const std::shared_ptr<IntermediateStorage> storage = toolchain->buildPrecompiledHeader(headerPath, pchPath, buildFlags);

  const bool built = pchPath.recheckExists() && storage && storage->getErrors().empty();
  if(!built) {
    LOG_WARNING(L"Could not precompile the shared headers of \"{}\"; indexing without them.", name);
    FileSystem::remove(pchPath);
    return {};
  }

  return includePchFlagsFor(pchPath);
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

    return includePchFlagsFor(pchOutputFilePath);
  }

  return {};
}
}    // namespace utility
