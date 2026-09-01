#include "data/indexer/grpc/GrpcIndexer.h"

#include <thread>

#include <fmt/format.h>

#include <grpcpp/grpcpp.h>

#include "app/LanguagePackageManager.h"
#include "Convert.h"
#include "data/indexer/IndexerComposite.h"
#include "indexer_worker.grpc.pb.h"
#include "logging.h"
#include "ScopedFunctor.h"
#include "utilityString.h"

GrpcIndexer::GrpcIndexer(std::string engineEndpoint, Id processId)
    : mEngineEndpoint(std::move(engineEndpoint)), mProcessId(processId) {}

void GrpcIndexer::work() {
  grpc::ChannelArguments channelArgs;
  channelArgs.SetMaxReceiveMessageSize(grpc_indexer::UnlimitedMessageSize);
  channelArgs.SetMaxSendMessageSize(grpc_indexer::UnlimitedMessageSize);
  auto channel = grpc::CreateCustomChannel(mEngineEndpoint, grpc::InsecureChannelCredentials(), channelArgs);
  auto stub = sourcetrail::IndexerWorkerService::NewStub(channel);

  auto pIndexer = LanguagePackageManager::getInstance()->instantiateSupportedIndexers();

  // Watch interrupt in a background thread. The context is hoisted out so the
  // main loop can cancel the (otherwise blocking) server-streaming RPC on normal
  // completion; without this the watcher thread would block forever in Read().
  bool interruptReceived = false;
  grpc::ClientContext watchCtx;

  std::thread watcherThread([&]() {
    sourcetrail::WatchInterruptRequest req;
    req.set_process_id(static_cast<uint64_t>(mProcessId));

    auto stream = stub->WatchInterrupt(&watchCtx, req);
    sourcetrail::InterruptEvent event;
    while(stream->Read(&event)) {
      if(event.interrupted()) {
        LOG_INFO(fmt::format("{} received interrupt via gRPC", mProcessId));
        interruptReceived = true;
        if(pIndexer) {
          pIndexer->interrupt();
        }
        break;
      }
    }
  });

  const ScopedFunctor watcherStopper([&]() {
    watchCtx.TryCancel();
    watcherThread.join();
  });

  LOG_INFO(fmt::format("{} GrpcIndexer starting up", mProcessId));

  while(!interruptReceived) {
    // Pull next command
    grpc::ClientContext ctx;
    sourcetrail::PullCommandRequest pullReq;
    pullReq.set_process_id(static_cast<uint64_t>(mProcessId));
    sourcetrail::PullCommandResponse pullResp;

    grpc::Status status = stub->PullCommand(&ctx, pullReq, &pullResp);
    if(!status.ok()) {
      LOG_ERROR(fmt::format("{} PullCommand failed: {}", mProcessId, status.error_message()));
      break;
    }

    if(!pullResp.command_found()) {
      if(pullResp.queue_closed()) {
        LOG_INFO(fmt::format("{} no more commands, shutting down", mProcessId));
        break;
      }
      // The queue is only empty for now. PullCommand already blocked on our behalf, so this
      // retries immediately instead of tearing down a process that is about to get work.
      continue;
    }

    const sourcetrail::IndexerCommand& cmdMsg = pullResp.command();
    auto pCommand = proto::convert::fromProto(cmdMsg);
    if(!pCommand) {
      continue;
    }

    // Report start
    {
      grpc::ClientContext statusCtx;
      sourcetrail::StatusReport report;
      report.set_process_id(static_cast<uint64_t>(mProcessId));
      report.set_event(sourcetrail::StatusReport::START_FILE);
      report.set_file_path(cmdMsg.source_file_path());
      sourcetrail::StatusReportResponse statusResp;
      stub->ReportStatus(&statusCtx, report, &statusResp);
    }

    LOG_INFO(fmt::format("{} indexing: {}", mProcessId, cmdMsg.source_file_path()));
    auto pResult = pIndexer->index(pCommand);

    bool pushFailed = false;
    if(pResult && !interruptReceived) {
      // Push result
      grpc::ClientContext pushCtx;
      sourcetrail::PushIntermediateStorageRequest pushReq;
      pushReq.set_process_id(static_cast<uint64_t>(mProcessId));
      *pushReq.mutable_storage() = proto::convert::toProto(*pResult);
      sourcetrail::PushIntermediateStorageResponse pushResp;
      grpc::Status pushStatus = stub->PushIntermediateStorage(&pushCtx, pushReq, &pushResp);
      if(!pushStatus.ok()) {
        pushFailed = true;
        LOG_ERROR(fmt::format("{} PushIntermediateStorage failed: {}", mProcessId, pushStatus.error_message()));
      }
    }

    // Report finish -- but only if the engine actually got the symbols. Reporting FINISH_FILE for
    // a push that failed counts the file as indexed while none of its symbols made it into the
    // database; leaving it unreported instead routes it through the engine's failed-file path, so
    // the user sees an error rather than a file that silently lost its contents.
    if(!pushFailed) {
      grpc::ClientContext statusCtx;
      sourcetrail::StatusReport report;
      report.set_process_id(static_cast<uint64_t>(mProcessId));
      report.set_event(sourcetrail::StatusReport::FINISH_FILE);
      sourcetrail::StatusReportResponse statusResp;
      stub->ReportStatus(&statusCtx, report, &statusResp);
    }
  }

  // Report done
  {
    grpc::ClientContext statusCtx;
    sourcetrail::StatusReport report;
    report.set_process_id(static_cast<uint64_t>(mProcessId));
    report.set_event(sourcetrail::StatusReport::PROCESS_DONE);
    sourcetrail::StatusReportResponse statusResp;
    stub->ReportStatus(&statusCtx, report, &statusResp);
  }

  LOG_INFO(fmt::format("{} GrpcIndexer shut down", mProcessId));
}
