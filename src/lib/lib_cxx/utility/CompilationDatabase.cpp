#include "CompilationDatabase.h"

#include <set>
#include <string>
#include <utility>

#include <fmt/format.h>

#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/JSONCompilationDatabase.h>

#include "FilePath.h"
#include "logging.h"
#include "utility.h"
#include "utilityString.h"

utility::CompilationDatabase::CompilationDatabase(FilePath filePath) : mFilePath(std::move(filePath)) {
  init();
}

std::vector<FilePath> utility::CompilationDatabase::getAllHeaderPaths() const {
  std::vector<FilePath> paths = utility::concat(mHeaders, mSystemHeaders);
  paths = utility::unique(paths);
  return paths;
}

std::vector<FilePath> utility::CompilationDatabase::getHeaderPaths() const {
  return mHeaders;
}

std::vector<FilePath> utility::CompilationDatabase::getSystemHeaderPaths() const {
  return mSystemHeaders;
}

std::vector<FilePath> utility::CompilationDatabase::getFrameworkHeaderPaths() const {
  return mFrameworkHeaders;
}

void utility::CompilationDatabase::init() {
  if(mFilePath.empty()) {
    LOG_WARNING("CompilationDatabase filePath is empty nothing to parse.");
    return;
  }

  if(!mFilePath.exists()) {
    LOG_WARNING(fmt::format("CompilationDatabase \"{}\" is missing.", mFilePath.str()));
    return;
  }

  std::string error;
  const std::shared_ptr<clang::tooling::JSONCompilationDatabase> cdb(clang::tooling::JSONCompilationDatabase::loadFromFile(
      utility::encodeToUtf8(mFilePath.wstr()), error, clang::tooling::JSONCommandLineSyntax::AutoDetect));

  if(!cdb) {
    LOG_ERROR(L"Loading compilation database from file \"" + mFilePath.wstr() + L"\" failed with error: " +
              utility::decodeFromUtf8(error));
    return;
  }

  std::vector<clang::tooling::CompileCommand> commands = cdb->getAllCompileCommands();
  std::set<FilePath> frameworkHeaders;
  std::set<FilePath> systemHeaders;
  std::set<FilePath> headers;

  {
    const std::wstring frameworkIncludeFlag = L"-iframework";
    const std::wstring systemIncludeFlag = L"-isystem";
    const std::wstring quoteFlag = L"-iquote";
    const std::wstring includeFlag = L"-I";
    for(auto& command : commands) {
      const std::wstring commandDirectory = utility::decodeFromUtf8(command.Directory);
      for(size_t i = 0; i < command.CommandLine.size(); i++) {
        std::wstring argument = utility::decodeFromUtf8(command.CommandLine[i]);
        if(i + 1 < command.CommandLine.size() && !utility::isPrefix<std::string>("-", command.CommandLine[i + 1])) {
          argument += utility::decodeFromUtf8(command.CommandLine[++i]);
        }

        if(utility::isPrefix(frameworkIncludeFlag, argument)) {
          frameworkHeaders.insert(
              FilePath(utility::trim(argument.substr(frameworkIncludeFlag.size())), commandDirectory).makeCanonical());
        } else if(utility::isPrefix(systemIncludeFlag, argument)) {
          systemHeaders.insert(FilePath(utility::trim(argument.substr(systemIncludeFlag.size())), commandDirectory).makeCanonical());
        } else if(utility::isPrefix(quoteFlag, argument)) {
          headers.insert(FilePath(utility::trim(argument.substr(quoteFlag.size())), commandDirectory).makeCanonical());
        } else if(utility::isPrefix(includeFlag, argument)) {
          headers.insert(FilePath(utility::trim(argument.substr(includeFlag.size())), commandDirectory).makeCanonical());
        }
      }
    }
  }

  mHeaders = utility::toVector(headers);
  mFrameworkHeaders = utility::toVector(frameworkHeaders);
  mSystemHeaders = utility::toVector(systemHeaders);
}