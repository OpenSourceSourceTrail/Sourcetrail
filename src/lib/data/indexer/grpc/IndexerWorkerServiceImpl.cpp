#include "IndexerWorkerServiceImpl.h"

#include <fmt/format.h>

#include "Convert.h"
#include "FilePath.h"
#include "IntermediateStorage.h"
#include "StorageProvider.h"
#include "logging.h"
#include "utilityString.h"

IndexerWorkerServiceImpl::IndexerWorkerServiceImpl(std::shared_ptr<StorageProvider> storageProvider)
    : mStorageProvider(std::move(storageProvider)) {}

void IndexerWorkerServiceImpl::fillCommands(std::vector<sourcetrail::IndexerCommand> commands) {
  const std::lock_guard<std::mutex> lock(mCommandMutex);
  for(auto& cmd : commands) {
    mCommandQueue.push_back(std::move(cmd));
  }
}

size_t IndexerWorkerServiceImpl::pendingCommandCount() {
  const std::lock_guard<std::mutex> lock(mCommandMutex);
  return mCommandQueue.size();
}

void IndexerWorkerServiceImpl::setInterrupted(bool interrupted) {
  mInterrupted.store(interrupted);
  notifyInterruptListeners();
}

std::vector<FilePath> IndexerWorkerServiceImpl::drainAndGetCrashedFiles() {
  // Called from TaskBuildIndex::doExit *after* all worker threads/processes are joined,
  // so no further gRPC calls can mutate state here. Any source file still recorded as
  // "in progress" means its worker died without reporting FINISH/DONE -> crashed.
  const std::lock_guard<std::mutex> lock(mStatusMutex);

  std::vector<FilePath> crashed = std::move(mCrashedFiles);
  for(auto& [processId, file] : mCurrentFileByProcess) {
    if(!file.empty()) {
      crashed.emplace_back(utility::decodeFromUtf8(file));
    }
  }
  return crashed;
}

size_t IndexerWorkerServiceImpl::getIndexedFileCount() const {
  return mIndexedFileCount.load();
}

size_t IndexerWorkerServiceImpl::getStartedFileCount() const {
  return mStartedFileCount.load();
}

std::vector<FilePath> IndexerWorkerServiceImpl::getCurrentlyIndexedSourceFilePaths() {
  const std::lock_guard<std::mutex> lock(mStatusMutex);
  std::vector<FilePath> paths;
  for(auto& [pid, file] : mCurrentFileByProcess) {
    if(!file.empty()) {
      paths.emplace_back(utility::decodeFromUtf8(file));
    }
  }
  return paths;
}

grpc::Status IndexerWorkerServiceImpl::PullCommand(grpc::ServerContext* /*ctx*/,
                                                   const sourcetrail::PullCommandRequest* /*req*/,
                                                   sourcetrail::PullCommandResponse* resp) {
  const std::lock_guard<std::mutex> lock(mCommandMutex);

  if(mInterrupted.load() || mCommandQueue.empty()) {
    resp->set_command_found(false);
    return grpc::Status::OK;
  }

  *resp->mutable_command() = std::move(mCommandQueue.front());
  mCommandQueue.pop_front();
  resp->set_command_found(true);

  return grpc::Status::OK;
}

grpc::Status IndexerWorkerServiceImpl::PushIntermediateStorage(grpc::ServerContext* /*ctx*/,
                                                               const sourcetrail::PushIntermediateStorageRequest* req,
                                                               sourcetrail::PushIntermediateStorageResponse* /*resp*/) {
  auto storage = proto::convert::fromProto(req->storage());
  if(storage) {
    LOG_INFO(fmt::format("IndexerWorkerService: received IntermediateStorage from process {}", req->process_id()));
    mStorageProvider->insert(storage);
    mIndexedFileCount++;
  }
  return grpc::Status::OK;
}

grpc::Status IndexerWorkerServiceImpl::ReportStatus(grpc::ServerContext* /*ctx*/,
                                                    const sourcetrail::StatusReport* req,
                                                    sourcetrail::StatusReportResponse* /*resp*/) {
  const uint64_t pid = req->process_id();

  switch(req->event()) {
  case sourcetrail::StatusReport::START_FILE: {
    const std::lock_guard<std::mutex> lock(mStatusMutex);
    const std::string prev = mCurrentFileByProcess[pid];
    if(!prev.empty()) {
      mCrashedFiles.emplace_back(utility::decodeFromUtf8(prev));
    }
    mCurrentFileByProcess[pid] = req->file_path();
    mStartedFileCount++;
    LOG_INFO(fmt::format("IndexerWorkerService: process {} started indexing {}", pid, req->file_path()));
    break;
  }
  case sourcetrail::StatusReport::FINISH_FILE: {
    const std::lock_guard<std::mutex> lock(mStatusMutex);
    mCurrentFileByProcess.erase(pid);
    LOG_INFO(fmt::format("IndexerWorkerService: process {} finished file", pid));
    break;
  }
  case sourcetrail::StatusReport::PROCESS_DONE: {
    const std::lock_guard<std::mutex> lock(mStatusMutex);
    mCurrentFileByProcess.erase(pid);
    LOG_INFO(fmt::format("IndexerWorkerService: process {} done", pid));
    break;
  }
  default:
    break;
  }
  return grpc::Status::OK;
}

grpc::Status IndexerWorkerServiceImpl::WatchInterrupt(grpc::ServerContext* ctx,
                                                      const sourcetrail::WatchInterruptRequest* /*req*/,
                                                      grpc::ServerWriter<sourcetrail::InterruptEvent>* writer) {
  {
    const std::lock_guard<std::mutex> lock(mInterruptListenersMutex);
    mInterruptListeners.push_back(writer);
  }

  // Send current state immediately so late-joiners don't miss a pre-existing interrupt
  if(mInterrupted.load()) {
    sourcetrail::InterruptEvent event;
    event.set_interrupted(true);
    writer->Write(event);
  }

  // Block until the client disconnects or the context is cancelled
  ctx->AsyncNotifyWhenDone(nullptr);
  while(!ctx->IsCancelled() && !mInterrupted.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  if(mInterrupted.load()) {
    sourcetrail::InterruptEvent event;
    event.set_interrupted(true);
    writer->Write(event);
  }

  {
    const std::lock_guard<std::mutex> lock(mInterruptListenersMutex);
    mInterruptListeners.erase(std::remove(mInterruptListeners.begin(), mInterruptListeners.end(), writer),
                              mInterruptListeners.end());
  }

  return grpc::Status::OK;
}

void IndexerWorkerServiceImpl::notifyInterruptListeners() {
  const std::lock_guard<std::mutex> lock(mInterruptListenersMutex);
  sourcetrail::InterruptEvent event;
  event.set_interrupted(true);
  for(auto* writer : mInterruptListeners) {
    writer->Write(event);
  }
}
