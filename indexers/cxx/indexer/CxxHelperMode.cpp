#include "CxxHelperMode.h"

#include <cstdlib>
#include <fstream>

#include "Convert.h"
#include "data/storage/IntermediateStorage.h"
#include "indexer_helper.pb.h"
#include "logging.h"
#include "project/CxxToolchainLocal.h"
#include "utilityString.h"

namespace helper {

int run(const std::string& requestFilePath, const std::string& responseFilePath) {
  sourcetrail::CxxToolchainRequest request;
  {
    std::ifstream stream(requestFilePath, std::ios::binary);
    if(!stream || !request.ParseFromIstream(&stream)) {
      LOG_ERROR("Could not read the helper request at " + requestFilePath);
      return EXIT_FAILURE;
    }
  }

  const CxxToolchainLocal toolchain;
  sourcetrail::CxxToolchainResponse response;

  switch(request.request_case()) {
  case sourcetrail::CxxToolchainRequest::kLoadCompilationDatabase: {
    const FilePath cdbPath(utility::decodeFromUtf8(request.load_compilation_database().cdb_path()));
    std::string error;
    if(const std::optional<std::vector<CxxCompileCommand>> commands = toolchain.loadCompilationDatabase(cdbPath, &error)) {
      response.set_ok(true);
      for(const CxxCompileCommand& command : *commands) {
        sourcetrail::CxxCompileCommandProto* message = response.add_compile_commands();
        message->set_directory(command.directory);
        message->set_file(command.file);
        for(const std::string& argument : command.arguments) {
          message->add_arguments(argument);
        }
      }
    } else {
      response.set_error(error.empty() ? "Unable to open and read the provided compilation database file." : error);
    }
    break;
  }
  case sourcetrail::CxxToolchainRequest::kBuildPrecompiledHeader: {
    const sourcetrail::BuildPrecompiledHeaderRequest& body = request.build_precompiled_header();
    std::vector<std::wstring> compilerFlags;
    compilerFlags.reserve(static_cast<size_t>(body.compiler_flags_size()));
    for(const std::string& flag : body.compiler_flags()) {
      compilerFlags.push_back(utility::decodeFromUtf8(flag));
    }

    const std::shared_ptr<IntermediateStorage> storage = toolchain.buildPrecompiledHeader(
        FilePath(utility::decodeFromUtf8(body.pch_input_file_path())),
        FilePath(utility::decodeFromUtf8(body.pch_output_file_path())),
        compilerFlags);
    if(storage) {
      response.set_ok(true);
      *response.mutable_storage() = proto::convert::toProto(*storage);
    } else {
      response.set_error("The precompiled header could not be generated.");
    }
    break;
  }
  case sourcetrail::CxxToolchainRequest::REQUEST_NOT_SET:
    response.set_error("The helper request names no operation.");
    break;
  }

  std::ofstream stream(responseFilePath, std::ios::binary);
  if(!stream || !response.SerializeToOstream(&stream)) {
    LOG_ERROR("Could not write the helper response to " + responseFilePath);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

}    // namespace helper
