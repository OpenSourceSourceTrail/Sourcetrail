#include <gtest/gtest.h>

#include "indexing/logic/grpc/IndexerWorkerServiceImpl.h"

namespace {
sourcetrail::StatusReport startFile(uint64_t processId, const std::string& path) {
  sourcetrail::StatusReport report;
  report.set_process_id(processId);
  report.set_event(sourcetrail::StatusReport::START_FILE);
  report.set_file_path(path);
  return report;
}
}    // namespace

TEST(IndexerWorkerServiceImpl, everyStartedFileIsReportedExactlyOnce) {
  IndexerWorkerServiceImpl service{nullptr};
  sourcetrail::StatusReportResponse response;

  const sourcetrail::StatusReport first = startFile(1, "/src/a.cpp");
  const sourcetrail::StatusReport second = startFile(2, "/src/b.cpp");
  ASSERT_TRUE(service.ReportStatus(nullptr, &first, &response).ok());
  ASSERT_TRUE(service.ReportStatus(nullptr, &second, &response).ok());

  const std::vector<FilePath> started = service.drainStartedSourceFilePaths();
  ASSERT_EQ(2, started.size());
  EXPECT_EQ(FilePath{L"/src/a.cpp"}, started[0]);
  EXPECT_EQ(FilePath{L"/src/b.cpp"}, started[1]);

  // Both files are still being indexed, but they have already been reported: draining again while
  // nothing new started is what used to repeat the same line in the GUI log on every poll.
  EXPECT_TRUE(service.drainStartedSourceFilePaths().empty());
}
