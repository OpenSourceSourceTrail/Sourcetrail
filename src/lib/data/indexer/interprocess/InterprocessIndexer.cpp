#include "InterprocessIndexer.h"
// fmt
#include <fmt/format.h>
// internal
#include "IndexerCommand.h"
#include "IndexerComposite.h"
#include "LanguagePackageManager.h"
#include "logging.h"
#include "ScopedFunctor.h"

InterprocessIndexer::InterprocessIndexer(const std::string& uuid, Id processId)
    : m_interprocessIndexerCommandManager(uuid, processId, false)
    , m_interprocessIndexingStatusManager(uuid, processId, false)
    , m_interprocessIntermediateStorageManager(uuid, processId, false)
    , m_uuid(uuid)
    , m_processId(processId) {}

void InterprocessIndexer::work() {
  bool updaterThreadRunning = true;
  std::shared_ptr<std::thread> pUpdaterThread;
  std::shared_ptr<IndexerBase> pIndexer;

  try {
    LOG_INFO(fmt::format("{} starting up indexer", m_processId));
    pIndexer = LanguagePackageManager::getInstance()->instantiateSupportedIndexers();

    pUpdaterThread = std::make_shared<std::thread>([&]() {
      while(updaterThreadRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if(m_interprocessIndexingStatusManager.getIndexingInterrupted()) {
          LOG_INFO(fmt::format("{} received indexer interrupt command.", m_processId));
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

    while(auto pIndexerCommand = m_interprocessIndexerCommandManager.popIndexerCommand()) {
      LOG_INFO(fmt::format("{} fetched indexer command for \"{}\"", m_processId, pIndexerCommand->getSourceFilePath().str()));
      LOG_INFO(fmt::format("{} indexer commands left: {}", m_processId, m_interprocessIndexerCommandManager.indexerCommandCount()));

      while(updaterThreadRunning) {
        const size_t storageCount = m_interprocessIntermediateStorageManager.getIntermediateStorageCount();
        if(storageCount < 2) {
          break;
        }

        LOG_INFO(fmt::format("{} waits, too many intermediate storages: {}", m_processId, storageCount));

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }

      if(!updaterThreadRunning) {
        break;
      }

      LOG_INFO(fmt::format("{} updating indexer status with currently indexed filepath", m_processId));
      m_interprocessIndexingStatusManager.startIndexingSourceFile(pIndexerCommand->getSourceFilePath());

      LOG_INFO(fmt::format("{} starting to index current file", m_processId));
      auto pResult = pIndexer->index(pIndexerCommand);

      if(pResult) {
        LOG_INFO(fmt::format("{} spushing index to shared memory", m_processId));
        m_interprocessIntermediateStorageManager.pushIntermediateStorage(pResult);
      }

      LOG_INFO(fmt::format("{} sfinalizing indexer status for current file", m_processId));
      m_interprocessIndexingStatusManager.finishIndexingSourceFile();

      LOG_INFO(fmt::format("{} sall done", m_processId));
    }
  } catch(boost::interprocess::interprocess_exception& e) {
    LOG_INFO(fmt::format("{} error: {}", m_processId, e.what()));
    throw e;
  } catch(std::exception& e) {
    LOG_INFO(fmt::format("{} error: {}", m_processId, e.what()));
    throw e;
  } catch(...) {
    LOG_INFO(fmt::format("{} something went wrong while running the indexer", m_processId));
  }

  LOG_INFO(fmt::format("{} shutting down indexer", m_processId));
}