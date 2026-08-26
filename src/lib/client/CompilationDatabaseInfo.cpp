#include "CompilationDatabaseInfo.h"

#include "Capabilities.h"
#include "engine.pb.h"
#include "EngineCall.h"
#include "ProtoJson.h"
#include "utilityString.h"

namespace client {

CompilationDatabaseInfo inspectCompilationDatabase(const FilePath& cdbPath) {
  sourcetrail::CompilationDatabaseInfoRequest request;
  request.set_cdb_path(utility::encodeToUtf8(cdbPath.wstr()));

  const std::optional<sourcetrail::CompilationDatabaseInfoResponse> response = call<sourcetrail::CompilationDatabaseInfoResponse>(
      Capabilities::instance().channel(),
      "inspectCompilationDatabase",
      "POST",
      "/api/v1/compilation-database",
      proto::json::toJson(request));

  CompilationDatabaseInfo info;
  if(!response) {
    info.error = "The indexing engine is not available, so the compilation database cannot be read.";
    return info;
  }

  info.valid = response->valid();
  info.error = response->error();
  info.containsIncludePchFlags = response->contains_include_pch_flags();
  for(const std::string& path : response->source_files()) {
    info.sourceFiles.emplace_back(utility::decodeFromUtf8(path));
  }
  for(const std::string& path : response->header_paths()) {
    info.headerPaths.emplace_back(utility::decodeFromUtf8(path));
  }
  return info;
}

}    // namespace client
