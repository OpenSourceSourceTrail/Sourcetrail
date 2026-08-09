#include "ConvertEvents.h"

#include "FilePath.h"
#include "utilityString.h"

namespace proto::convert {

sourcetrail::EngineEvent toIndexingStartedEvent() {
  sourcetrail::EngineEvent event;
  event.mutable_indexing_started();
  return event;
}

sourcetrail::EngineEvent toIndexingProgressEvent(size_t startedFileCount,
                                                 size_t finishedFileCount,
                                                 size_t totalFileCount,
                                                 const std::vector<FilePath>& sourcePaths) {
  sourcetrail::EngineEvent event;
  sourcetrail::IndexingProgressEvent* progress = event.mutable_indexing_progress();
  progress->set_started_file_count(startedFileCount);
  progress->set_finished_file_count(finishedFileCount);
  progress->set_total_file_count(totalFileCount);
  for(const FilePath& path : sourcePaths) {
    progress->add_source_paths(utility::encodeToUtf8(path.wstr()));
  }
  return event;
}

sourcetrail::EngineEvent toIndexingFinishedEvent(size_t indexedFileCount,
                                                 size_t totalIndexedFileCount,
                                                 size_t completedFileCount,
                                                 size_t totalFileCount,
                                                 float time,
                                                 ErrorCountInfo errorInfo,
                                                 bool interrupted,
                                                 bool shallow) {
  sourcetrail::EngineEvent event;
  sourcetrail::IndexingFinishedEvent* finished = event.mutable_indexing_finished();
  finished->set_indexed_file_count(indexedFileCount);
  finished->set_total_indexed_file_count(totalIndexedFileCount);
  finished->set_completed_file_count(completedFileCount);
  finished->set_total_file_count(totalFileCount);
  finished->set_time_seconds(time);
  finished->set_error_total(errorInfo.total);
  finished->set_error_fatal(errorInfo.fatal);
  finished->set_interrupted(interrupted);
  finished->set_shallow(shallow);
  return event;
}

sourcetrail::EngineEvent toStatusInfoEvent(const std::wstring& message, bool isError) {
  sourcetrail::EngineEvent event;
  sourcetrail::StatusInfoEvent* status = event.mutable_status_info();
  status->set_message(utility::encodeToUtf8(message));
  status->set_is_error(isError);
  return event;
}

sourcetrail::EngineEvent toErrorCountEvent(size_t total, size_t fatal) {
  sourcetrail::EngineEvent event;
  sourcetrail::ErrorCountEvent* count = event.mutable_error_count();
  count->set_total(total);
  count->set_fatal(fatal);
  return event;
}

sourcetrail::EngineEvent toUnknownProgressEvent(const std::wstring& title, const std::wstring& message) {
  sourcetrail::EngineEvent event;
  sourcetrail::UnknownProgressEvent* progress = event.mutable_unknown_progress();
  progress->set_title(utility::encodeToUtf8(title));
  progress->set_message(utility::encodeToUtf8(message));
  return event;
}

sourcetrail::EngineEvent toProgressEvent(const std::wstring& title, const std::wstring& message, size_t progress) {
  sourcetrail::EngineEvent event;
  sourcetrail::ProgressEvent* known = event.mutable_progress();
  known->set_title(utility::encodeToUtf8(title));
  known->set_message(utility::encodeToUtf8(message));
  known->set_progress(progress);
  return event;
}

sourcetrail::EngineEvent toClearDialogsEvent() {
  sourcetrail::EngineEvent event;
  event.mutable_clear_dialogs();
  return event;
}

std::vector<FilePath> fromProtoSourcePaths(const sourcetrail::IndexingProgressEvent& event) {
  std::vector<FilePath> paths;
  paths.reserve(static_cast<size_t>(event.source_paths_size()));
  for(const std::string& path : event.source_paths()) {
    paths.emplace_back(utility::decodeFromUtf8(path));
  }
  return paths;
}

}    // namespace proto::convert
