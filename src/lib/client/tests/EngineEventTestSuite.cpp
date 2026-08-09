#include <gtest/gtest.h>

#include "ConvertEvents.h"
#include "FilePath.h"
#include "utilityString.h"

// The file counts are the piece that silently produced 0% for a whole release: they exist only in
// the DialogView call and in this event, nowhere on either process's message bus.
TEST(EngineEventTestSuite, indexingProgressCarriesCountsAndPaths) {
  const std::vector<FilePath> sourcePaths{FilePath(L"/src/main.cpp"), FilePath(L"/src/other.cpp")};

  const sourcetrail::EngineEvent event = proto::convert::toIndexingProgressEvent(3, 7, 100, sourcePaths);

  ASSERT_EQ(sourcetrail::EngineEvent::kIndexingProgress, event.event_case());
  EXPECT_EQ(3U, event.indexing_progress().started_file_count());
  EXPECT_EQ(7U, event.indexing_progress().finished_file_count());
  EXPECT_EQ(100U, event.indexing_progress().total_file_count());
  EXPECT_EQ(sourcePaths, proto::convert::fromProtoSourcePaths(event.indexing_progress()));
}

TEST(EngineEventTestSuite, indexingProgressWithoutFilesIsStillWellFormed) {
  const sourcetrail::EngineEvent event = proto::convert::toIndexingProgressEvent(0, 0, 0, {});

  ASSERT_EQ(sourcetrail::EngineEvent::kIndexingProgress, event.event_case());
  EXPECT_EQ(0U, event.indexing_progress().total_file_count());
  EXPECT_TRUE(proto::convert::fromProtoSourcePaths(event.indexing_progress()).empty());
}

TEST(EngineEventTestSuite, indexingFinishedCarriesCountsAndErrors) {
  const sourcetrail::EngineEvent event = proto::convert::toIndexingFinishedEvent(
      12, 20, 18, 30, 4.5F, ErrorCountInfo{5, 2}, true, false);

  ASSERT_EQ(sourcetrail::EngineEvent::kIndexingFinished, event.event_case());
  const sourcetrail::IndexingFinishedEvent& finished = event.indexing_finished();
  EXPECT_EQ(12U, finished.indexed_file_count());
  EXPECT_EQ(20U, finished.total_indexed_file_count());
  EXPECT_EQ(18U, finished.completed_file_count());
  EXPECT_EQ(30U, finished.total_file_count());
  EXPECT_FLOAT_EQ(4.5F, finished.time_seconds());
  EXPECT_EQ(5U, finished.error_total());
  EXPECT_EQ(2U, finished.error_fatal());
  EXPECT_TRUE(finished.interrupted());
  EXPECT_FALSE(finished.shallow());
}

TEST(EngineEventTestSuite, statusInfoSurvivesNonAsciiText) {
  const sourcetrail::EngineEvent event = proto::convert::toStatusInfoEvent(L"Indexing… ünicode", true);

  ASSERT_EQ(sourcetrail::EngineEvent::kStatusInfo, event.event_case());
  EXPECT_EQ(L"Indexing… ünicode", utility::decodeFromUtf8(event.status_info().message()));
  EXPECT_TRUE(event.status_info().is_error());
}
