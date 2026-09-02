#include "data/TaskFinishParsing.h"

#include <chrono>
#include <thread>
#include <utility>

#include "Blackboard.h"
#include "component/view/DialogView.h"
#include "data/storage/PersistentStorage.h"
#include "MessageQueue.h"
#include "status/messages/MessageStatus.h"
#include "TimeStamp.h"
#include "type/indexing/MessageIndexingFinished.h"
#include "type/indexing/MessageIndexingStatus.h"
#include "utilityString.h"

namespace {
// Something unrelated (an IDE ping, say) can keep the queue busy indefinitely; the report dialog
// matters more than a perfectly drained queue.
constexpr std::chrono::milliseconds MaxQueueDrainWait{2000};

// The report dialog reaches the UI thread directly, while every progress and log message takes the
// message queue first. Without this drain the dialog overtakes whatever is still queued, and those
// messages then land behind it -- the log keeps scrolling and the status bar progress keeps
// climbing while the run is already reported as finished.
void drainMessageQueue() {
  if(!IMessageQueue::hasInstance()) {
    return;
  }

  const auto deadline = std::chrono::steady_clock::now() + MaxQueueDrainWait;
  while(IMessageQueue::getInstanceRaw()->hasMessagesQueued() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}
}    // namespace

TaskFinishParsing::TaskFinishParsing(std::shared_ptr<PersistentStorage> storage, std::shared_ptr<DialogView> dialogView)
    : m_storage(std::move(storage)), m_dialogView(std::move(dialogView)) {}

void TaskFinishParsing::terminate() {
  m_dialogView->clearDialogs();

  MessageStatus(L"An unknown exception was thrown during indexing.", true, false).dispatch();
  MessageIndexingFinished().dispatch();
}

void TaskFinishParsing::doEnter(std::shared_ptr<Blackboard> /*blackboard*/) {
  m_storage->setMode(SqliteIndexStorage::STORAGE_MODE_READ);
}

Task::TaskState TaskFinishParsing::doUpdate(std::shared_ptr<Blackboard> blackboard) {
  TimeStamp start = TimeStamp::now();

  m_dialogView->showUnknownProgressDialog(L"Finish Indexing", L"Optimizing database");
  m_storage->optimizeMemory();
  m_dialogView->hideUnknownProgressDialog();

  double time = TimeStamp::durationSeconds(start);

  if(blackboard->exists("clear_time")) {
    float clearTime = 0;
    blackboard->get("clear_time", clearTime);
    time += static_cast<double>(clearTime);
  }

  if(blackboard->exists("index_time")) {
    float indexTime = 0;
    blackboard->get("index_time", indexTime);
    time += static_cast<double>(indexTime);
  }

  int indexedSourceFileCount = 0;
  blackboard->get("indexed_source_file_count", indexedSourceFileCount);

  int sourceFileCount = 0;
  blackboard->get("source_file_count", sourceFileCount);

  bool interruptedIndexing = false;
  blackboard->get("interrupted_indexing", interruptedIndexing);

  bool shallowIndexing = false;
  blackboard->get("shallow_indexing", shallowIndexing);

  ErrorCountInfo errorInfo = m_storage->getErrorCount();

  std::wstring status;
  status += L"Finished indexing: ";
  status += std::to_wstring(indexedSourceFileCount) + L"/" + std::to_wstring(sourceFileCount) + L" source files indexed; ";
  status += utility::decodeFromUtf8(TimeStamp::secondsToString(time));
  status += L"; " + std::to_wstring(errorInfo.total) + L" error" + (errorInfo.total != 1 ? L"s" : L"");
  if(errorInfo.fatal > 0) {
    status += L" (" + std::to_wstring(errorInfo.fatal) + L" fatal)";
  }
  MessageStatus(status, false, false).dispatch();

  // Hide the progress bar before the dialog, not after it: dispatched here it is the last thing the
  // status bar sees, instead of arriving once the user has already closed the report.
  MessageIndexingStatus(false).dispatch();
  drainMessageQueue();

  StorageStats stats = m_storage->getStorageStats();
  DatabasePolicy policy = m_dialogView->finishedIndexingDialog(static_cast<size_t>(indexedSourceFileCount),
                                                               static_cast<size_t>(sourceFileCount),
                                                               stats.completedFileCount,
                                                               stats.fileCount,
                                                               static_cast<float>(time),
                                                               errorInfo,
                                                               interruptedIndexing,
                                                               shallowIndexing);

  if(policy == DATABASE_POLICY_KEEP) {
    blackboard->set("keep_database", true);
  } else if(policy == DATABASE_POLICY_DISCARD) {
    blackboard->set("discard_database", true);
  } else if(policy == DATABASE_POLICY_REFRESH) {
    blackboard->set("keep_database", true);
    blackboard->set("refresh_database", true);
  }

  return STATE_SUCCESS;
}

void TaskFinishParsing::doExit(std::shared_ptr<Blackboard> /*blackboard*/) {
  m_storage.reset();
}

void TaskFinishParsing::doReset(std::shared_ptr<Blackboard> /*blackboard*/) {}
