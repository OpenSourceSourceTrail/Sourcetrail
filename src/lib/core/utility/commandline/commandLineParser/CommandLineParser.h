#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "FilePath.h"
#include "project/RefreshInfo.h"

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

  /**
   * Whether --engine was given: the GUI reads the index from a separate engine process over HTTP
   * rather than hosting one itself. Without it the GUI owns the index in-process, with no HTTP hop.
   */
  [[nodiscard]] bool usesRemoteEngine() const;

  /**
   * The engine to attach to, as "host:port,token", or empty when --engine was given with no value:
   * the GUI then spawns and supervises an engine of its own, as it used to.
   */
  [[nodiscard]] const std::string& getEngineEndpoint() const;

  /**
   * Port the in-process engine should serve HTTP on, or nullopt for no listener at all. 0 asks for
   * an ephemeral port. Opt-in on purpose: a loopback port is reachable by every local process and
   * by any page the user's browser loads, so a GUI nobody asked to be a server opens none.
   */
  [[nodiscard]] std::optional<uint16_t> getHttpPort() const;

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
  bool m_useRemoteEngine = false;
  std::string m_engineEndpoint;
  std::optional<uint16_t> m_httpPort;

  bool m_quit = false;
  bool m_withoutGUI = false;

  std::wstring m_errorString;
};

}    // namespace commandline
