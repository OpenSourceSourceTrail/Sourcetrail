#include "CxxToolchainRemote.h"

#include <filesystem>
#include <fstream>

#include <fmt/format.h>

#include "Convert.h"
#include "indexer_helper.pb.h"
#include "IndexerPluginRegistry.h"
#include "IntermediateStorage.h"
#include "logging.h"
#include "utilityApp.h"
#include "utilityString.h"
#include "utilityUuid.h"

namespace {

/** A precompiled header is a real compile; it must not share the short default process timeout. */
constexpr int NoTimeout = -1;

std::optional<sourcetrail::CxxToolchainResponse> runHelper(const sourcetrail::CxxToolchainRequest& request, std::string* error) {
  const std::optional<IndexerPluginRegistry::Plugin> plugin = IndexerPluginRegistry::getInstance()->pluginFor(INDEXER_COMMAND_CXX);
  if(!plugin || !plugin->indexerExecutablePath.exists()) {
    if(error != nullptr) {
      *error = "No C/C++ indexer is installed, so compilation databases cannot be read.";
    }
    return std::nullopt;
  }

  const std::filesystem::path scratch = std::filesystem::temp_directory_path() / ("sourcetrail_helper_" + utility::getUuidString());
  const std::filesystem::path requestPath = scratch.string() + ".req";
  const std::filesystem::path responsePath = scratch.string() + ".res";

  struct Cleanup {
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove(request, ignored);
      std::filesystem::remove(response, ignored);
    }
    const std::filesystem::path& request;
    const std::filesystem::path& response;
  } const cleanup{requestPath, responsePath};

  {
    std::ofstream stream(requestPath, std::ios::binary);
    if(!stream || !request.SerializeToOstream(&stream)) {
      if(error != nullptr) {
        *error = "Could not write the request for the C/C++ indexer helper.";
      }
      return std::nullopt;
    }
  }

  std::vector<std::wstring> arguments;
  if(!plugin->launcherPath.empty()) {
    arguments.insert(arguments.end(), plugin->launcherArgs.begin(), plugin->launcherArgs.end());
    arguments.push_back(plugin->indexerExecutablePath.wstr());
  }
  arguments.emplace_back(L"--helper");
  arguments.push_back(utility::decodeFromUtf8(requestPath.string()));
  arguments.push_back(utility::decodeFromUtf8(responsePath.string()));

  const std::wstring processPath = plugin->launcherPath.empty() ? plugin->indexerExecutablePath.wstr() :
                                                                  plugin->launcherPath.wstr();

  const utility::ProcessOutput result = utility::executeProcess(processPath, arguments, FilePath(), false, NoTimeout);
  if(result.exitCode != 0) {
    if(error != nullptr) {
      *error = fmt::format("The C/C++ indexer helper failed with exit code {}.", result.exitCode);
    }
    return std::nullopt;
  }

  std::ifstream stream(responsePath, std::ios::binary);
  sourcetrail::CxxToolchainResponse response;
  if(!stream || !response.ParseFromIstream(&stream)) {
    if(error != nullptr) {
      *error = "The C/C++ indexer helper produced no readable answer.";
    }
    return std::nullopt;
  }

  if(!response.ok()) {
    if(error != nullptr) {
      *error = response.error();
    }
    return std::nullopt;
  }

  return response;
}

}    // namespace

std::optional<std::vector<CxxCompileCommand>> CxxToolchainRemote::loadCompilationDatabase(const FilePath& cdbPath,
                                                                                          std::string* error) const {
  sourcetrail::CxxToolchainRequest request;
  request.mutable_load_compilation_database()->set_cdb_path(utility::encodeToUtf8(cdbPath.wstr()));

  const std::optional<sourcetrail::CxxToolchainResponse> response = runHelper(request, error);
  if(!response) {
    return std::nullopt;
  }

  std::vector<CxxCompileCommand> commands;
  commands.reserve(static_cast<size_t>(response->compile_commands_size()));
  for(const sourcetrail::CxxCompileCommandProto& command : response->compile_commands()) {
    commands.emplace_back(
        CxxCompileCommand{command.directory(), command.file(), {command.arguments().begin(), command.arguments().end()}});
  }
  return commands;
}

std::shared_ptr<IntermediateStorage> CxxToolchainRemote::buildPrecompiledHeader(
    const FilePath& pchInputFilePath, const FilePath& pchOutputFilePath, const std::vector<std::wstring>& compilerFlags) const {
  sourcetrail::CxxToolchainRequest request;
  sourcetrail::BuildPrecompiledHeaderRequest* body = request.mutable_build_precompiled_header();
  body->set_pch_input_file_path(utility::encodeToUtf8(pchInputFilePath.wstr()));
  body->set_pch_output_file_path(utility::encodeToUtf8(pchOutputFilePath.wstr()));
  for(const std::wstring& flag : compilerFlags) {
    body->add_compiler_flags(utility::encodeToUtf8(flag));
  }

  std::string error;
  const std::optional<sourcetrail::CxxToolchainResponse> response = runHelper(request, &error);
  if(!response) {
    LOG_ERROR("Could not generate the precompiled header: " + error);
    return {};
  }

  return proto::convert::fromProto(response->storage());
}
