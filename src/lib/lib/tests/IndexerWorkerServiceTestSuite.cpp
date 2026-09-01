#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "data/indexer/grpc/IndexerWorkerServiceImpl.h"
#include "data/storage/StorageProvider.h"
#include "FilePath.h"

using namespace std::chrono_literals;

namespace {

std::vector<sourcetrail::IndexerCommand> oneCommand(const std::string& sourceFilePath) {
  sourcetrail::IndexerCommand command;
  command.set_source_file_path(sourceFilePath);
  std::vector<sourcetrail::IndexerCommand> commands;
  commands.push_back(std::move(command));
  return commands;
}

IndexerWorkerServiceImpl makeService() {
  return IndexerWorkerServiceImpl{std::make_shared<StorageProvider>()};
}

sourcetrail::PullCommandResponse pull(IndexerWorkerServiceImpl& service) {
  const sourcetrail::PullCommandRequest request;
  sourcetrail::PullCommandResponse response;
  EXPECT_TRUE(service.PullCommand(nullptr, &request, &response).ok());
  return response;
}

}    // namespace

TEST(IndexerWorkerService, emptyOpenQueueParksInsteadOfEndingTheWorker) {
  IndexerWorkerServiceImpl service = makeService();

  const auto startedAt = std::chrono::steady_clock::now();
  const sourcetrail::PullCommandResponse response = pull(service);
  const auto waited = std::chrono::steady_clock::now() - startedAt;

  EXPECT_FALSE(response.command_found());
  // Not closed: the worker must retry rather than tear its process down.
  EXPECT_FALSE(response.queue_closed());
  EXPECT_GE(waited, IndexerWorkerServiceImpl::PullWait / 2);
}

TEST(IndexerWorkerService, aParkedPullWakesOnTheNextRefill) {
  IndexerWorkerServiceImpl service = makeService();

  std::thread producer([&service]() {
    std::this_thread::sleep_for(50ms);
    service.fillCommands(oneCommand("main.cpp"));
  });

  const auto startedAt = std::chrono::steady_clock::now();
  const sourcetrail::PullCommandResponse response = pull(service);
  const auto waited = std::chrono::steady_clock::now() - startedAt;
  producer.join();

  EXPECT_TRUE(response.command_found());
  EXPECT_EQ("main.cpp", response.command().source_file_path());
  EXPECT_LT(waited, IndexerWorkerServiceImpl::PullWait);
}

TEST(IndexerWorkerService, closingTheQueueStillHandsOutWhatIsPending) {
  IndexerWorkerServiceImpl service = makeService();
  service.fillCommands(oneCommand("main.cpp"));
  service.closeQueue();

  const sourcetrail::PullCommandResponse first = pull(service);
  EXPECT_TRUE(first.command_found());
  EXPECT_EQ("main.cpp", first.command().source_file_path());

  const sourcetrail::PullCommandResponse second = pull(service);
  EXPECT_FALSE(second.command_found());
  EXPECT_TRUE(second.queue_closed());
  EXPECT_TRUE(service.isQueueClosed());
}

TEST(IndexerWorkerService, interruptDiscardsPendingWorkAndReleasesTheWorker) {
  IndexerWorkerServiceImpl service = makeService();
  service.fillCommands(oneCommand("main.cpp"));
  service.setInterrupted(true);

  const sourcetrail::PullCommandResponse response = pull(service);
  EXPECT_FALSE(response.command_found());
  EXPECT_TRUE(response.queue_closed());
}
