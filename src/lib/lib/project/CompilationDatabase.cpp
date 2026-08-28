#include "project/CompilationDatabase.h"

#include <set>
#include <string>

#include "FilePath.h"
#include "project/utilitySourceGroupCxx.h"
#include "utility.h"
#include "utilityString.h"

namespace utility {

CompilationDatabase::CompilationDatabase(const std::vector<CxxCompileCommand>& commands) {
  const std::wstring frameworkIncludeFlag = L"-iframework";
  const std::wstring systemIncludeFlag = L"-isystem";
  const std::wstring quoteFlag = L"-iquote";
  const std::wstring includeFlag = L"-I";

  std::set<FilePath> frameworkHeaders;
  std::set<FilePath> systemHeaders;
  std::set<FilePath> headers;

  for(const CxxCompileCommand& command : commands) {
    const std::wstring commandDirectory = utility::decodeFromUtf8(command.directory);
    for(size_t i = 0; i < command.arguments.size(); i++) {
      std::wstring argument = utility::decodeFromUtf8(command.arguments[i]);
      // A flag and its value may arrive as one argument or as two; join the pair before matching.
      if(i + 1 < command.arguments.size() && !utility::isPrefix<std::string>("-", command.arguments[i + 1])) {
        argument += utility::decodeFromUtf8(command.arguments[++i]);
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

  mHeaders = utility::toVector(headers);
  mFrameworkHeaders = utility::toVector(frameworkHeaders);
  mSystemHeaders = utility::toVector(systemHeaders);
}

CompilationDatabase::CompilationDatabase(const FilePath& cdbPath)
    : CompilationDatabase(utility::loadCompilationDatabase(cdbPath).value_or(std::vector<CxxCompileCommand>{})) {}

std::vector<FilePath> CompilationDatabase::getAllHeaderPaths() const {
  return utility::unique(utility::concat(mHeaders, mSystemHeaders));
}

std::vector<FilePath> CompilationDatabase::getHeaderPaths() const {
  return mHeaders;
}

std::vector<FilePath> CompilationDatabase::getSystemHeaderPaths() const {
  return mSystemHeaders;
}

std::vector<FilePath> CompilationDatabase::getFrameworkHeaderPaths() const {
  return mFrameworkHeaders;
}

}    // namespace utility
