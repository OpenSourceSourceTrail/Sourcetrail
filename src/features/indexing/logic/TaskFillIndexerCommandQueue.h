#ifndef TASK_FILL_INDEXER_COMMAND_QUEUE_H
#define TASK_FILL_INDEXER_COMMAND_QUEUE_H

#include <memory>
#include <queue>

#include "FilePath.h"
#include "indexing/messages/MessageIndexingInterrupted.h"
#include "MessageListener.h"
#include "Task.h"

class IndexerCommandProvider;
class IndexerWorkerServiceImpl;

class TaskFillIndexerCommandsQueue
    : public Task
    , public MessageListener<MessageIndexingInterrupted> {
public:
  TaskFillIndexerCommandsQueue(std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
                               std::unique_ptr<IndexerCommandProvider> indexerCommandProvider,
                               size_t maximumQueueSize);

protected:
  void doEnter(std::shared_ptr<Blackboard> blackboard) override;
  TaskState doUpdate(std::shared_ptr<Blackboard> blackboard) override;
  void doExit(std::shared_ptr<Blackboard> blackboard) override;
  void doReset(std::shared_ptr<Blackboard> blackboard) override;
  void terminate() override;

  void handleMessage(MessageIndexingInterrupted* message) override;

  bool fillCommandQueue();

private:
  std::shared_ptr<IndexerWorkerServiceImpl> m_indexerWorkerService;
  std::unique_ptr<IndexerCommandProvider> m_indexerCommandProvider;

  const size_t m_maximumQueueSize;

  std::queue<FilePath> m_filePathQueue;
  std::mutex m_commandsMutex;

  bool m_interrupted = false;
};

#endif    // TASK_FILL_INDEXER_COMMAND_QUEUE_H
