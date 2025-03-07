#pragma once
#include <memory>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "FilePath.h"
#include "RefreshInfo.h"

namespace commandline {
class CommandlineCommand;

class CommandLineParser final {
public:
  explicit CommandLineParser(std::string version);

  CommandLineParser(const CommandLineParser&) = delete;
  CommandLineParser& operator=(const CommandLineParser&) = delete;
  CommandLineParser(CommandLineParser&&) = delete;
  CommandLineParser& operator=(CommandLineParser&&) = delete;

  ~CommandLineParser();

  void preparse(std::vector<std::string> args);

  // todo: update implementation [SOUR-138]
  void parse();

  [[nodiscard]] bool runWithoutGUI() const;

  [[nodiscard]] bool exitApplication() const;

  [[nodiscard]] bool hasError() const;

  std::wstring getError();

  void fullRefresh();

  void incompleteRefresh();

  void setShallowIndexingRequested(bool enabled = true);

  [[nodiscard]] const FilePath& getProjectFilePath() const;

  void setProjectFile(const FilePath& filepath);

  [[nodiscard]] RefreshMode getRefreshMode() const;

  [[nodiscard]] bool getShallowIndexingRequested() const;

private:
  void processProjectfile();

  void printHelp() const;

  boost::program_options::options_description m_options;
  boost::program_options::positional_options_description m_positional;

  std::vector<std::shared_ptr<CommandlineCommand>> m_commands;
  std::vector<std::string> m_args;

  const std::string m_version;
  FilePath m_projectFile;
  RefreshMode m_refreshMode = RefreshMode::UpdatedFiles;
  bool m_shallowIndexingRequested = false;

  bool m_quit = false;
  bool m_withoutGUI = false;

  std::wstring m_errorString;
};

}    // namespace commandline
