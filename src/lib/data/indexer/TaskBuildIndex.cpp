#include "TaskBuildIndex.h"

#include <spdlog/spdlog.h>

#include "AppPath.h"
#include "Blackboard.h"
#include "DialogView.h"
#include "InterprocessIndexer.h"
#include "ParserClientImpl.h"
#include "StorageProvider.h"
#include "TimeStamp.h"
#include "type/indexing/MessageIndexingStatus.h"
#include "UserPaths.h"
#include "utilityApp.h"

namespace {
constexpr auto DelayTimeBeforeFinishUpdateInMs = 50;
constexpr auto DelayTimeBeforeStatrWorkInMs = 200;
constexpr auto DelayTimeInMs = 100;
constexpr auto MaxProcessTimeInMs = 500;
constexpr int MaxStorageCount = 10;
}    // namespace

TaskBuildIndex::TaskBuildIndex(size_t processCount,
                               std::shared_ptr<StorageProvider> storageProvider,
                               std::shared_ptr<DialogView> dialogView,
                               std::string appUUID,
                               bool multiProcessIndexing)
    : mStorageProvider(std::move(storageProvider))
    , mDialogView(std::move(dialogView))
    , mAppUUID(std::move(appUUID))
    , mMultiProcessIndexing(multiProcessIndexing)
    , mInterprocessIndexingStatusManager(mAppUUID, 0, true)
    , mProcessCount(processCount) {}

void TaskBuildIndex::doEnter(std::shared_ptr<Blackboard> blackboard) {
  mInterprocessIndexingStatusManager.setIndexingInterrupted(false);

  mIndexingFileCount = 0;
  updateIndexingDialog(blackboard, std::vector<FilePath>());

  // FIXME(Hussein): Multiprocess needs the file to log
  // const std::wstring logFilePath;
  // Logger* logger = LogManager::getInstance()->getLoggerByType("FileLogger");
  // if(logger) {
  //   logFilePath = dynamic_cast<FileLogger*>(logger)->getLogFilePath().wstr();
  // }

  // start indexer processes
  for(size_t index = 0; index < mProcessCount; ++index) {
    {
      const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
      ++mRunningThreadCount;
    }

    const size_t processId = index + 1;    // 0 remains reserved for the main process

    mInterprocessIntermediateStorageManagers.push_back(
        std::make_shared<InterprocessIntermediateStorageManager>(mAppUUID, processId, true));

    if(mMultiProcessIndexing) {
      mProcessThreads.push_back(
          std::make_unique<std::thread>(&TaskBuildIndex::runIndexerProcess, this, processId, std::wstring{} /*logFilePath*/));
    } else {
      mProcessThreads.push_back(std::make_unique<std::thread>(&TaskBuildIndex::runIndexerThread, this, processId));
    }
  }

  blackboard->set<bool>("indexer_threads_started", true);
}

Task::TaskState TaskBuildIndex::doUpdate(std::shared_ptr<Blackboard> blackboard) {
  size_t runningThreadCount = 0;
  {
    const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
    runningThreadCount = mRunningThreadCount;
  }

  blackboard->get<bool>("indexer_command_queue_stopped", mIndexerCommandQueueStopped);

  const std::vector<FilePath> indexingFiles = mInterprocessIndexingStatusManager.getCurrentlyIndexedSourceFilePaths();
  if(!indexingFiles.empty()) {
    updateIndexingDialog(blackboard, indexingFiles);
  }

  if(mIndexerCommandQueueStopped && runningThreadCount == 0) {
    LOG_INFO("command queue stopped and no running threads. done.");
    return STATE_SUCCESS;
  } else if(mInterrupted) {
    LOG_INFO("interrupted indexing.");
    blackboard->set("interrupted_indexing", true);
    return STATE_SUCCESS;
  }

  if(fetchIntermediateStorages(blackboard)) {
    updateIndexingDialog(blackboard, std::vector<FilePath>());
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

  if(!mInterrupted) {
    while(fetchIntermediateStorages(blackboard)) {}
  }

  if(const std::vector<FilePath> crashedFiles = mInterprocessIndexingStatusManager.getCrashedSourceFilePaths();
     !crashedFiles.empty()) {
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

  blackboard->set<bool>("indexer_threads_stopped", true);
}

void TaskBuildIndex::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {}

void TaskBuildIndex::terminate() {
  mInterrupted = true;
  utility::killRunningProcesses();
}

void TaskBuildIndex::handleMessage(MessageIndexingInterrupted* /*message*/) {
  LOG_INFO("sending indexer interrupt command.");

  mInterprocessIndexingStatusManager.setIndexingInterrupted(true);
  mInterrupted = true;

  mDialogView->showUnknownProgressDialog(L"Interrupting Indexing", L"Waiting for indexer\nthreads to finish");
}

void TaskBuildIndex::runIndexerProcess(int processId, const std::wstring& logFilePath) {
  const FilePath indexerProcessPath = AppPath::getCxxIndexerFilePath();
  if(!indexerProcessPath.exists()) {
    mInterrupted = true;
    LOG_ERROR(L"Cannot start indexer process because executable is missing at \"" + indexerProcessPath.wstr() + L"\"");
    return;
  }

  std::vector<std::wstring> commandArguments;
  commandArguments.push_back(std::to_wstring(processId));
  commandArguments.push_back(utility::decodeFromUtf8(mAppUUID));
  commandArguments.push_back(AppPath::getSharedDataDirectoryPath().getAbsolute().wstr());
  commandArguments.push_back(UserPaths::getUserDataDirectoryPath().getAbsolute().wstr());

  if(!logFilePath.empty()) {
    commandArguments.push_back(logFilePath);
  }

  int result = 1;
  while((!mIndexerCommandQueueStopped || result != 0) && !mInterrupted) {
    result = utility::executeProcess(indexerProcessPath.wstr(), commandArguments, FilePath(), false, -1).exitCode;

    LOG_INFO(fmt::format("Indexer process {} returned with {}", processId, std::to_string(result)));
  }

  {
    const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
    mRunningThreadCount--;
  }
}

void TaskBuildIndex::runIndexerThread(int processId) {
  do {    // NOLINT(cppcoreguidelines-avoid-do-while)
    InterprocessIndexer indexer(mAppUUID, static_cast<Id>(processId));
    indexer.work();    // this will only return if there are no indexer commands left in the queue
    if(!mInterrupted) {
      // sleeping if interrupted may result in a crash due to objects that are already
      // destroyed after waking up again
      std::this_thread::sleep_for(std::chrono::milliseconds(DelayTimeBeforeStatrWorkInMs));
    }
  } while(!mIndexerCommandQueueStopped && !mInterrupted);

  {
    const std::lock_guard<std::mutex> lock(mRunningThreadCountMutex);
    mRunningThreadCount--;
  }
}

bool TaskBuildIndex::fetchIntermediateStorages(const std::shared_ptr<Blackboard>& blackboard) {
  int poppedStorageCount = 0;

  if(const int providerStorageCount = mStorageProvider->getStorageCount(); providerStorageCount > MaxStorageCount) {
    LOG_INFO("waiting, too many storages queued: {}", providerStorageCount);

    std::this_thread::sleep_for(std::chrono::milliseconds(DelayTimeInMs));

    return true;
  }

  const TimeStamp currentTime = TimeStamp::now();
  do {    // NOLINT(cppcoreguidelines-avoid-do-while)
    const Id finishedProcessId = mInterprocessIndexingStatusManager.getNextFinishedProcessId();
    if(0 == finishedProcessId || finishedProcessId > mInterprocessIntermediateStorageManagers.size()) {
      break;
    }

    const std::shared_ptr<InterprocessIntermediateStorageManager>& storageManager =
        mInterprocessIntermediateStorageManagers[finishedProcessId - 1];

    const size_t storageCount = storageManager->getIntermediateStorageCount();
    if(0 == storageCount) {
      break;
    }

    LOG_INFO("{} - storage count: {}", storageManager->getProcessId(), storageCount);
    mStorageProvider->insert(storageManager->popIntermediateStorage());
    poppedStorageCount++;
  } while(TimeStamp::now().deltaMS(currentTime) <
          MaxProcessTimeInMs);    // don't process all storages at once to allow for status updates in-between

  if(poppedStorageCount > 0) {
    blackboard->update<int>("indexed_source_file_count", [=](int count) { return count + poppedStorageCount; });
    return true;
  }

  return false;
}

void TaskBuildIndex::updateIndexingDialog(const std::shared_ptr<Blackboard>& blackboard, const std::vector<FilePath>& sourcePaths) {
  // TODO: factor in unindexed files...
  int sourceFileCount = 0;
  int indexedSourceFileCount = 0;
  blackboard->get("source_file_count", sourceFileCount);
  blackboard->get("indexed_source_file_count", indexedSourceFileCount);

  mIndexingFileCount += sourcePaths.size();

  mDialogView->updateIndexingDialog(
      mIndexingFileCount, static_cast<size_t>(indexedSourceFileCount), static_cast<size_t>(sourceFileCount), sourcePaths);

  const size_t progress = (sourceFileCount > 0) ? 0 : static_cast<size_t>(indexedSourceFileCount * 100 / sourceFileCount);
  MessageIndexingStatus{true, progress}.dispatch();
}
