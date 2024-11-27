#include "InterprocessIndexer.h"

#include <fmt/format.h>

#include "IndexerCommand.h"
#include "IndexerComposite.h"
#include "LanguagePackageManager.h"
#include "logging.h"
#include "ScopedFunctor.h"

InterprocessIndexer::InterprocessIndexer(const std::string& uuid, Id processId)
    : mInterprocessIndexerCommandManager(uuid, processId, false)
    , mInterprocessIndexingStatusManager(uuid, processId, false)
    , mInterprocessIntermediateStorageManager(uuid, processId, false)
    , mUuid(uuid)
    , mProcessId(processId) {}

void InterprocessIndexer::work() {
  bool updaterThreadRunning = true;
  std::shared_ptr<std::thread> pUpdaterThread;
  std::shared_ptr<IndexerBase> pIndexer;

  try {
    LOG_INFO(fmt::format("{} starting up indexer", mProcessId));
    pIndexer = LanguagePackageManager::getInstance()->instantiateSupportedIndexers();

    pUpdaterThread = std::make_shared<std::thread>([&]() {
      while(updaterThreadRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if(mInterprocessIndexingStatusManager.getIndexingInterrupted()) {
          LOG_INFO(fmt::format("{} received indexer interrupt command.", mProcessId));
          if(pIndexer) {
            pIndexer->interrupt();
          }
          updaterThreadRunning = false;
        }
      }
    });

    ScopedFunctor threadStopper([&]() {
      updaterThreadRunning = false;
      if(pUpdaterThread) {
        pUpdaterThread->join();
        pUpdaterThread.reset();
      }
    });

    while(auto pIndexerCommand = mInterprocessIndexerCommandManager.popIndexerCommand()) {
      LOG_INFO(fmt::format("{} fetched indexer command for \"{}\"", mProcessId, pIndexerCommand->getSourceFilePath().str()));
      LOG_INFO(fmt::format("{} indexer commands left: {}", mProcessId, mInterprocessIndexerCommandManager.indexerCommandCount()));

      while(updaterThreadRunning) {
        const size_t storageCount = mInterprocessIntermediateStorageManager.getIntermediateStorageCount();
        if(storageCount < 2) {
          break;
        }

        LOG_INFO(fmt::format("{} waits, too many intermediate storages: {}", mProcessId, storageCount));

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }

      if(!updaterThreadRunning) {
        break;
      }

      LOG_INFO(fmt::format("{} updating indexer status with currently indexed filepath", mProcessId));
      mInterprocessIndexingStatusManager.startIndexingSourceFile(pIndexerCommand->getSourceFilePath());

      LOG_INFO(fmt::format("{} starting to index current file", mProcessId));
      auto pResult = pIndexer->index(pIndexerCommand);

      if(pResult) {
        LOG_INFO(fmt::format("{} spushing index to shared memory", mProcessId));
        mInterprocessIntermediateStorageManager.pushIntermediateStorage(pResult);
      }

      LOG_INFO(fmt::format("{} sfinalizing indexer status for current file", mProcessId));
      mInterprocessIndexingStatusManager.finishIndexingSourceFile();

      LOG_INFO(fmt::format("{} sall done", mProcessId));
    }
  } catch(boost::interprocess::interprocess_exception& e) {
    LOG_INFO(fmt::format("{} error: {}", mProcessId, e.what()));
    throw e;
  } catch(std::exception& e) {
    LOG_INFO(fmt::format("{} error: {}", mProcessId, e.what()));
    throw e;
  } catch(...) {
    LOG_INFO(fmt::format("{} something went wrong while running the indexer", mProcessId));
  }

  LOG_INFO(fmt::format("{} shutting down indexer", mProcessId));
}