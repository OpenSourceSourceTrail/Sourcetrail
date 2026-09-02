#include "indexing/logic/grpc/GrpcIndexer.h"

#include <chrono>
#include <cstdio>
#include <thread>

#include <fmt/format.h>

#include <grpcpp/grpcpp.h>

#include "app/LanguagePackageManager.h"
#include "Convert.h"
#include "indexer_worker.grpc.pb.h"
#include "indexing/logic/IndexerComposite.h"
#include "logging.h"
#include "ScopedFunctor.h"
#include "utilityString.h"

GrpcIndexer::GrpcIndexer(std::string engineEndpoint, Id processId)
    : mEngineEndpoint(std::move(engineEndpoint)), mProcessId(processId) {}

namespace {

using WorkerClock = std::chrono::steady_clock;

/**
 * Wall-clock split of one worker's life: how much went into Clang, into building the protobuf
 * message, and into the RPC that carries it back.
 *
 * Printed to stderr rather than logged, because worker logging is off unless verbose indexer
 * logging is enabled *and* a log path was passed -- and the log path never reaches the worker's
 * argv. stderr is drained by whoever spawned the process, so it always arrives.
 */
struct WorkerTimings {
  double parseMs = 0.0;
  double serializeMs = 0.0;
  double pushMs = 0.0;
  /** Blocked waiting for work. PullCommand parks server-side, so this is starvation, not polling. */
  double pullMs = 0.0;
  size_t files = 0;
};

double elapsedMs(WorkerClock::time_point start) {
  return std::chrono::duration<double, std::milli>(WorkerClock::now() - start).count();
}

}    // namespace

void GrpcIndexer::work() {
  grpc::ChannelArguments channelArgs;
  channelArgs.SetMaxReceiveMessageSize(grpc_indexer::UnlimitedMessageSize);
  channelArgs.SetMaxSendMessageSize(grpc_indexer::UnlimitedMessageSize);
  auto channel = grpc::CreateCustomChannel(mEngineEndpoint, grpc::InsecureChannelCredentials(), channelArgs);
  auto stub = sourcetrail::IndexerWorkerService::NewStub(channel);

  auto pIndexer = LanguagePackageManager::getInstance()->instantiateSupportedIndexers();

  WorkerTimings timings;

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

    const auto pullStart = WorkerClock::now();
    grpc::Status status = stub->PullCommand(&ctx, pullReq, &pullResp);
    timings.pullMs += elapsedMs(pullStart);
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
    const auto parseStart = WorkerClock::now();
    auto pResult = pIndexer->index(pCommand);
    timings.parseMs += elapsedMs(parseStart);
    ++timings.files;

    // Only a result that actually reached the engine counts. index() returning null means no
    // indexer handled the command at all, and reporting the file finished for that would count it
    // as indexed while none of its symbols exist.
    bool indexed = false;
    if(pResult && !interruptReceived) {
      // Push result
      grpc::ClientContext pushCtx;
      sourcetrail::PushIntermediateStorageRequest pushReq;
      pushReq.set_process_id(static_cast<uint64_t>(mProcessId));
      const auto serializeStart = WorkerClock::now();
      *pushReq.mutable_storage() = proto::convert::toProto(*pResult);
      timings.serializeMs += elapsedMs(serializeStart);

      sourcetrail::PushIntermediateStorageResponse pushResp;
      const auto pushStart = WorkerClock::now();
      grpc::Status pushStatus = stub->PushIntermediateStorage(&pushCtx, pushReq, &pushResp);
      timings.pushMs += elapsedMs(pushStart);
      if(pushStatus.ok()) {
        indexed = true;
      } else {
        LOG_ERROR(fmt::format("{} PushIntermediateStorage failed: {}", mProcessId, pushStatus.error_message()));
      }
    }

    // Report finish -- but only if the engine actually got the symbols. Reporting FINISH_FILE for
    // a push that failed, or for a command no indexer could run, counts the file as indexed while
    // none of its symbols made it into the database; leaving it unreported instead routes it
    // through the engine's failed-file path, so the user sees an error rather than a file that
    // silently lost its contents.
    if(indexed) {
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

  std::fprintf(stderr,
               "INDEXER_TIMING process=%llu files=%zu parse_ms=%.1f serialize_ms=%.1f push_ms=%.1f pull_ms=%.1f\n",
               static_cast<unsigned long long>(mProcessId),
               timings.files,
               timings.parseMs,
               timings.serializeMs,
               timings.pushMs,
               timings.pullMs);
  std::fflush(stderr);

  LOG_INFO(fmt::format("{} GrpcIndexer shut down", mProcessId));
}
