#include "TaskBuildIndex.h"

#include <fmt/format.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>

#include "AppPath.h"
#include "Blackboard.h"
#include "DialogView.h"
#include "GrpcIndexer.h"
#include "IndexerCommandType.h"
#include "IndexerPluginRegistry.h"
#include "ParserClientImpl.h"
#include "StorageProvider.h"
#include "TimeStamp.h"
#include "UserPaths.h"
#include "type/indexing/MessageIndexingStatus.h"
#include "utilityApp.h"
#include "utilityString.h"

namespace {
constexpr auto DelayTimeBeforeFinishUpdateInMs = 50;
constexpr auto DelayTimeBeforeStartWorkInMs = 200;

/** Below this, a non-zero exit means the worker never got going. */
constexpr std::chrono::milliseconds CrashLoopThreshold{2000};
/** Multiplied by the failure count, so the three attempts are spread over a few seconds. */
constexpr std::chrono::milliseconds RespawnBackoff{500};
}    // namespace

bool TaskBuildIndex::isCrashLoopExit(int exitCode, std::chrono::milliseconds ranFor) {
  return exitCode != 0 && ranFor < CrashLoopThreshold;
}

TaskBuildIndex::TaskBuildIndex(size_t processCount,
                               std::shared_ptr<IndexerWorkerServiceImpl> indexerWorkerService,
                               std::shared_ptr<StorageProvider> storageProvider,
                               std::shared_ptr<DialogView> dialogView,
                               std::string appUUID,
                               bool multiProcessIndexing,
                               IndexerCommandType commandType)
    : mStorageProvider(std::move(storageProvider))
    , mDialogView(std::move(dialogView))
    , mAppUUID(std::move(appUUID))
    , mMultiProcessIndexing(multiProcessIndexing)
    , mCommandType(commandType)
    , mIndexerWorkerService(std::move(indexerWorkerService))
    , mProcessCount(processCount) {}

void TaskBuildIndex::doEnter(std::shared_ptr<Blackboard> blackboard) {
  mLastReportedIndexedCount = 0;
  updateIndexingDialog(blackboard, std::vector<FilePath>());

  // Start gRPC server for worker boundary. The service instance is shared with
  // TaskFillIndexerCommandsQueue, which feeds its command queue.
  grpc::ServerBuilder builder;
  builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(), &mEnginePort);
  builder.RegisterService(mIndexerWorkerService.get());
  mGrpcServer = builder.BuildAndStart();

  LOG_INFO(fmt::format("IndexerWorkerService listening on localhost:{}", mEnginePort));

  // Fill command queue from blackboard (TaskFillIndexerCommandQueue places them there)
  // The actual command list is passed via the IndexerWorkerServiceImpl after this method,
  // but commands are populated by TaskFillIndexerCommandQueue calling into StorageProvider.
  // We defer to doUpdate to ensure the command queue is filled before workers start.

  // Start worker processes / threads
  for(size_t index = 0; index < mProcessCount; ++index) {
    {
      const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
      ++mRunningThreadCount;
    }

    const int processId = static_cast<int>(index + 1);

    if(mMultiProcessIndexing) {
      mProcessThreads.push_back(
          std::make_unique<std::thread>(&TaskBuildIndex::runIndexerProcess, this, processId, std::wstring{}));
    } else {
      mProcessThreads.push_back(std::make_unique<std::thread>(&TaskBuildIndex::runIndexerThread, this, processId));
    }
  }

  blackboard->set<bool>("indexer_threads_started", true);
}

Task::TaskState TaskBuildIndex::doUpdate(std::shared_ptr<Blackboard> blackboard) {
  // Read the stop flag before the thread count: a worker only exits normally once the flag is set, so
  // this order makes "no workers left, flag still clear" mean they all died rather than a lost race.
  blackboard->get<bool>("indexer_command_queue_stopped", mIndexerCommandQueueStopped);

  size_t runningThreadCount = 0;
  {
    const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
    runningThreadCount = mRunningThreadCount;
  }

  // Update progress from gRPC service counts
  if(mIndexerWorkerService) {
    const size_t indexedCount = mIndexerWorkerService->getIndexedFileCount();
    if(indexedCount > mLastReportedIndexedCount) {
      const size_t delta = indexedCount - mLastReportedIndexedCount;
      mLastReportedIndexedCount = indexedCount;
      blackboard->update<int>("indexed_source_file_count", [delta](int count) {
        return count + static_cast<int>(delta);
      });
    }

    const std::vector<FilePath> indexingFiles = mIndexerWorkerService->getCurrentlyIndexedSourceFilePaths();
    if(!indexingFiles.empty()) {
      updateIndexingDialog(blackboard, indexingFiles);
    }
  }

  if(mIndexerCommandQueueStopped && runningThreadCount == 0) {
    LOG_INFO("command queue stopped and no running threads. done.");
    return STATE_SUCCESS;
  } else if(mInterrupted) {
    LOG_INFO("interrupted indexing.");
    blackboard->set("interrupted_indexing", true);
    return STATE_SUCCESS;
  } else if(runningThreadCount == 0) {
    // Every worker gave up while there was still work queued. Failing here is the whole point: the
    // alternative is waiting forever for a queue nobody is draining.
    LOG_ERROR("All indexer workers stopped before the command queue was drained.");
    return STATE_FAILURE;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(DelayTimeBeforeFinishUpdateInMs));

  return STATE_RUNNING;
}

void TaskBuildIndex::doExit(std::shared_ptr<Blackboard> blackboard) {
  for(auto& processThread : mProcessThreads) {
    processThread->join();
    processThread.reset();
  }
  mProcessThreads.clear();

  if(mIndexerWorkerService && !mInterrupted) {
    const std::vector<FilePath> crashedFiles = mIndexerWorkerService->drainAndGetCrashedFiles();
    if(!crashedFiles.empty()) {
      const std::shared_ptr<IntermediateStorage> storage = std::make_shared<IntermediateStorage>();
      const std::shared_ptr<ParserClientImpl> parserClient = std::make_shared<ParserClientImpl>(storage.get());

      for(const FilePath& path : crashedFiles) {
        const Id fileId = parserClient->recordFile(path.getCanonical(), false);
        parserClient->recordError(
            L"The translation unit threw an exception during indexing. Please check if the "
            L"source file "
            "conforms to the specified language standard and all necessary options are defined "
            "within your project "
            "setup.",
            true,
            true,
            path,
            ParseLocation(fileId, 1, 1));
        LOG_INFO(L"crashed translation unit: " + path.wstr());
      }
      mStorageProvider->insert(storage);
    }
  }

  // Shut down gRPC server. requestShutdown() unblocks any outstanding
  // WatchInterrupt streams first, since Server::Shutdown() otherwise waits
  // forever for them to return on a normal, non-interrupted indexing run.
  if(mIndexerWorkerService) {
    mIndexerWorkerService->requestShutdown();
  }
  if(mGrpcServer) {
    mGrpcServer->Shutdown();
    mGrpcServer.reset();
  }
  mIndexerWorkerService.reset();

  blackboard->set<bool>("indexer_threads_stopped", true);
}

void TaskBuildIndex::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {}

void TaskBuildIndex::terminate() {
  mInterrupted = true;
  if(mIndexerWorkerService) {
    mIndexerWorkerService->setInterrupted(true);
  }
  utility::killRunningProcesses();
}

void TaskBuildIndex::handleMessage(MessageIndexingInterrupted* /*message*/) {
  LOG_INFO("sending indexer interrupt command via gRPC.");

  mInterrupted = true;
  if(mIndexerWorkerService) {
    mIndexerWorkerService->setInterrupted(true);
  }

  mDialogView->showUnknownProgressDialog(L"Interrupting Indexing", L"Waiting for indexer\nthreads to finish");
}

void TaskBuildIndex::runIndexerProcess(int processId, const std::wstring& /*logFilePath*/) {
  FilePath indexerProcessPath;
  FilePath launcherPath;
  std::vector<std::wstring> launcherArgs;
  // Every indexer, including the built-in C/C++ one, is resolved through its manifest. That is what
  // makes the plugin system real rather than cosmetic: one lookup path, no privileged indexer.
  if(std::optional<IndexerPluginRegistry::Plugin> plugin = IndexerPluginRegistry::getInstance()->pluginFor(mCommandType);
     plugin.has_value()) {
    indexerProcessPath = plugin->indexerExecutablePath;
    launcherPath = plugin->launcherPath;
    launcherArgs = plugin->launcherArgs;
  }
  if(!indexerProcessPath.exists()) {
    mInterrupted = true;
    LOG_ERROR(L"Cannot start indexer process because executable is missing at \"" + indexerProcessPath.wstr() + L"\"");
    const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
    mRunningThreadCount--;
    return;
  }

  const std::string endpoint = fmt::format("localhost:{}", mEnginePort);

  std::vector<std::wstring> commandArguments;
  if(!launcherPath.empty()) {
    commandArguments.insert(commandArguments.end(), launcherArgs.begin(), launcherArgs.end());
    commandArguments.push_back(indexerProcessPath.wstr());
  }
  commandArguments.push_back(std::to_wstring(processId));
  commandArguments.push_back(L"--engine-endpoint");
  commandArguments.push_back(utility::decodeFromUtf8(endpoint));
  commandArguments.push_back(AppPath::getSharedDataDirectoryPath().getAbsolute().wstr());
  commandArguments.push_back(UserPaths::getUserDataDirectoryPath().getAbsolute().wstr());

  const std::wstring processPath = launcherPath.empty() ? indexerProcessPath.wstr() : launcherPath.wstr();

  int result = 1;
  int consecutiveFailures = 0;
  while((!mIndexerCommandQueueStopped || result != 0) && !mInterrupted) {
    const auto startedAt = std::chrono::steady_clock::now();
    result = utility::executeProcess(processPath, commandArguments, FilePath(), false, -1).exitCode;
    const auto ranFor = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt);
    LOG_INFO(fmt::format("Indexer process {} returned with {} after {}ms", processId, result, ranFor.count()));

    if(!isCrashLoopExit(result, ranFor)) {
      consecutiveFailures = 0;
      continue;
    }

    if(++consecutiveFailures >= MaxConsecutiveWorkerFailures) {
      // Abandon this worker only. The others keep draining the queue, and if they all reach this point
      // doUpdate fails the task instead of hanging.
      LOG_ERROR(fmt::format("Indexer process {} died immediately {} times in a row; abandoning this worker. Indexer: {}",
                            processId,
                            consecutiveFailures,
                            indexerProcessPath.str()));
      break;
    }
    std::this_thread::sleep_for(RespawnBackoff * consecutiveFailures);
  }

  {
    const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
    mRunningThreadCount--;
  }
}

void TaskBuildIndex::runIndexerThread(int processId) {
  const std::string endpoint = fmt::format("localhost:{}", mEnginePort);

  do {
    GrpcIndexer indexer(endpoint, static_cast<Id>(processId));
    indexer.work();
    if(!mInterrupted) {
      std::this_thread::sleep_for(std::chrono::milliseconds(DelayTimeBeforeStartWorkInMs));
    }
  } while(!mIndexerCommandQueueStopped && !mInterrupted);

  {
    const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
    mRunningThreadCount--;
  }
}

void TaskBuildIndex::updateIndexingDialog(const std::shared_ptr<Blackboard>& blackboard, const std::vector<FilePath>& sourcePaths) {
  int sourceFileCount = 0;
  int indexedSourceFileCount = 0;
  blackboard->get("source_file_count", sourceFileCount);
  blackboard->get("indexed_source_file_count", indexedSourceFileCount);

  const size_t startedFileCount = mIndexerWorkerService ? mIndexerWorkerService->getStartedFileCount() : 0;

  mDialogView->updateIndexingDialog(
      startedFileCount, static_cast<size_t>(indexedSourceFileCount), static_cast<size_t>(sourceFileCount), sourcePaths);

  const size_t progress = (sourceFileCount > 0) ? static_cast<size_t>(indexedSourceFileCount * 100 / sourceFileCount) : 0;
  MessageIndexingStatus{true, progress}.dispatch();
}
