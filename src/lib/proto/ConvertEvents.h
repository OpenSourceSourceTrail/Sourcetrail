#pragma once
#include <string>
#include <vector>

#include "engine.pb.h"
#include "error/domain/ErrorCountInfo.h"

class FilePath;

/**
 * The engine event stream, in both directions.
 *
 * These live here rather than in either process so the two halves cannot drift: the engine builds
 * an EngineEvent from a DialogView call or a bus message, the client takes it apart again. The
 * counts are the whole point -- MessageIndexingStatus carries a percentage and nothing else, so
 * anything that wants "n of m files" has to come through here.
 */
namespace proto::convert {

sourcetrail::EngineEvent toIndexingStartedEvent();

sourcetrail::EngineEvent toIndexingProgressEvent(size_t startedFileCount,
                                                 size_t finishedFileCount,
                                                 size_t totalFileCount,
                                                 const std::vector<FilePath>& sourcePaths);

sourcetrail::EngineEvent toIndexingFinishedEvent(size_t indexedFileCount,
                                                 size_t totalIndexedFileCount,
                                                 size_t completedFileCount,
                                                 size_t totalFileCount,
                                                 float time,
                                                 ErrorCountInfo errorInfo,
                                                 bool interrupted,
                                                 bool shallow);

sourcetrail::EngineEvent toStatusInfoEvent(const std::wstring& message, bool isError);

sourcetrail::EngineEvent toErrorCountEvent(size_t total, size_t fatal);

sourcetrail::EngineEvent toUnknownProgressEvent(const std::wstring& title, const std::wstring& message);

sourcetrail::EngineEvent toProgressEvent(const std::wstring& title, const std::wstring& message, size_t progress);

sourcetrail::EngineEvent toClearDialogsEvent();

/** Source paths as the DialogView wants them again. */
std::vector<FilePath> fromProtoSourcePaths(const sourcetrail::IndexingProgressEvent& event);

}    // namespace proto::convert
