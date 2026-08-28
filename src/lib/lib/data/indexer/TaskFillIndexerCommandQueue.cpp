#include "data/indexer/TaskFillIndexerCommandQueue.h"

#include "../../../scheduling/Blackboard.h"
#include "Convert.h"
#include "data/indexer/grpc/IndexerWorkerServiceImpl.h"
#include "data/indexer/IndexerCommandProvider.h"
#include "FileSystem.h"
#include "logging.h"
#include "utilityFile.h"

TaskFillIndexerCommandsQueue::TaskFillIndexerCommandsQueue(std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
                                                           std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
                                                           size_t maximumQueueSize)
    : m_indexerWorkerService(std::move(indexerWorkerService))
    , m_indexerCommandProvider(std::move(indexerCommandProvider))
    , m_maximumQueueSize(maximumQueueSize) {}

void TaskFillIndexerCommandsQueue::doEnter(std::shared_ptr<Blackboard> blackboard) {
  {
    std::lock_guard<std::mutex> lock(m_commandsMutex);
    for(const FilePath& filePath : utility::partitionFilePathsBySize(m_indexerCommandProvider->getAllSourceFilePaths(), 2)) {
      m_filePathQueue.emplace(filePath);
    }
  }

  fillCommandQueue();

  blackboard->set<bool>("indexer_command_queue_started", true);
}

Task::TaskState TaskFillIndexerCommandsQueue::doUpdate(std::shared_ptr<Blackboard> /*blackboard*/) {
  if(m_interrupted) {
    return STATE_FAILURE;
  }

  if(!fillCommandQueue()) {
    std::lock_guard<std::mutex> lock(m_commandsMutex);

    if(m_indexerCommandProvider->empty()) {
      return STATE_SUCCESS;
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  return STATE_RUNNING;
}

void TaskFillIndexerCommandsQueue::doExit(std::shared_ptr<Blackboard> blackboard) {
  blackboard->set<bool>("indexer_command_queue_stopped", true);
}

void TaskFillIndexerCommandsQueue::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {
  m_interrupted = false;
}

void TaskFillIndexerCommandsQueue::terminate() {
  m_interrupted = true;
}

void TaskFillIndexerCommandsQueue::handleMessage(MessageIndexingInterrupted* /*message*/) {
  std::lock_guard<std::mutex> lock(m_commandsMutex);

  LOG_INFO("Discarding remaining " +
           std::to_string(m_indexerCommandProvider->size() + m_indexerWorkerService->pendingCommandCount()) +
           " indexer commands.");

  std::queue<FilePath> empty;
  std::swap(m_filePathQueue, empty);

  m_indexerCommandProvider->clear();

  LOG_INFO("Remaining: " + std::to_string(m_indexerCommandProvider->size() + m_indexerWorkerService->pendingCommandCount()) + ".");
}

bool TaskFillIndexerCommandsQueue::fillCommandQueue() {
  const size_t pending = m_indexerWorkerService->pendingCommandCount();
  if(pending >= m_maximumQueueSize) {
    return false;
  }
  const size_t refillAmount = m_maximumQueueSize - pending;

  std::lock_guard<std::mutex> lock(m_commandsMutex);
  std::vector<sourcetrail::IndexerCommand> commands;

  while(!m_indexerCommandProvider->empty() && commands.size() < refillAmount) {
    std::shared_ptr<IndexerCommand> command;
    if(!m_filePathQueue.empty()) {
      command = m_indexerCommandProvider->consumeCommandForSourceFilePath(m_filePathQueue.front());
      m_filePathQueue.pop();
    } else {
      command = m_indexerCommandProvider->consumeCommand();
    }
    if(command) {
      commands.push_back(proto::convert::toProto(command.get()));
    }
  }

  if(!commands.empty()) {
    m_indexerWorkerService->fillCommands(std::move(commands));
    return true;
  }

  return false;
}
