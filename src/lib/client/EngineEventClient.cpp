#include "EngineEventClient.h"

#include <chrono>
#include <memory>

#include "Application.h"
#include "ConvertEvents.h"
#include "engine.grpc.pb.h"
#include "EngineChannel.h"
#include "ErrorCountInfo.h"
#include "logging.h"
#include "type/error/MessageErrorCountUpdate.h"
#include "type/indexing/MessageIndexingFinished.h"
#include "type/indexing/MessageIndexingStarted.h"
#include "type/indexing/MessageIndexingStatus.h"
#include "type/MessageStatus.h"
#include "utilityString.h"

namespace client {

namespace {

constexpr std::chrono::milliseconds ReconnectDelay{500};

std::shared_ptr<DialogView> dialogView(DialogView::UseCase useCase) {
  Application* application = Application::getInstance().get();
  return application != nullptr ? application->getDialogView(useCase) : nullptr;
}

}    // namespace

EngineEventClient::EngineEventClient(EngineChannel* channel) : mChannel(channel) {}

EngineEventClient::~EngineEventClient() {
  stop();
}

void EngineEventClient::start() {
  if(mThread.joinable()) {
    return;
  }
  mStopping = false;
  mThread = std::thread(&EngineEventClient::run, this);
}

void EngineEventClient::stop() {
  if(!mThread.joinable()) {
    return;
  }

  mStopping = true;
  {
    const std::lock_guard<std::mutex> lock(mContextMutex);
    if(mContext != nullptr) {
      mContext->TryCancel();
    }
  }
  mThread.join();
}

void EngineEventClient::run() {
  const sourcetrail::WatchEventsRequest request;

  while(!mStopping) {
    sourcetrail::EngineService::Stub* stub = mChannel != nullptr ? mChannel->getStub() : nullptr;
    if(stub == nullptr) {
      std::this_thread::sleep_for(ReconnectDelay);
      continue;
    }

    grpc::ClientContext context;
    {
      const std::lock_guard<std::mutex> lock(mContextMutex);
      if(mStopping) {
        return;
      }
      mContext = &context;
    }

    std::unique_ptr<grpc::ClientReader<sourcetrail::EngineEvent>> reader = stub->WatchEvents(&context, request);
    sourcetrail::EngineEvent event;
    while(reader->Read(&event)) {
      apply(event);
    }
    std::ignore = reader->Finish();

    {
      const std::lock_guard<std::mutex> lock(mContextMutex);
      mContext = nullptr;
    }

    if(!mStopping) {
      // The engine went away; the supervisor is restarting it and the stub will point at the new
      // port once the handshake lands.
      std::this_thread::sleep_for(ReconnectDelay);
    }
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void EngineEventClient::apply(const sourcetrail::EngineEvent& event) {
  switch(event.event_case()) {
  case sourcetrail::EngineEvent::kIndexingStarted:
    MessageIndexingStarted{}.dispatch();
    break;

  case sourcetrail::EngineEvent::kIndexingProgress: {
    const sourcetrail::IndexingProgressEvent& progress = event.indexing_progress();
    if(auto view = dialogView(DialogView::UseCase::INDEXING); view) {
      view->updateIndexingDialog(progress.started_file_count(),
                                 progress.finished_file_count(),
                                 progress.total_file_count(),
                                 proto::convert::fromProtoSourcePaths(progress));
    }
    // The status bar wants a percentage and the engine's own MessageIndexingStatus never crosses
    // the boundary, so derive it from the counts that did.
    const size_t total = progress.total_file_count();
    const size_t percent = total > 0 ? progress.finished_file_count() * 100 / total : 0;
    MessageIndexingStatus{true, percent}.dispatch();
    break;
  }

  case sourcetrail::EngineEvent::kIndexingFinished: {
    const sourcetrail::IndexingFinishedEvent& finished = event.indexing_finished();
    MessageIndexingStatus{false}.dispatch();
    MessageIndexingFinished{}.dispatch();

    if(auto view = dialogView(DialogView::UseCase::INDEXING); view) {
      // QtDialogView::finishedIndexingDialog blocks until the user closes the report. On this thread
      // that would stop us reading, which backs the engine's writer up behind its listener mutex and
      // stalls the engine itself -- so the report gets its own thread.
      // ponytail: detached thread; give the reader a work queue if more blocking events appear.
      std::thread([view,
                   indexed = finished.indexed_file_count(),
                   totalIndexed = finished.total_indexed_file_count(),
                   completed = finished.completed_file_count(),
                   total = finished.total_file_count(),
                   seconds = finished.time_seconds(),
                   errors = ErrorCountInfo{finished.error_total(), finished.error_fatal()},
                   interrupted = finished.interrupted(),
                   shallow = finished.shallow()]() {
        std::ignore = view->finishedIndexingDialog(indexed, totalIndexed, completed, total, seconds, errors, interrupted, shallow);
      }).detach();
    }
    break;
  }

  case sourcetrail::EngineEvent::kIndexingError:
    MessageStatus{utility::decodeFromUtf8(event.indexing_error().error_message()), true}.dispatch();
    break;

  case sourcetrail::EngineEvent::kStatusInfo:
    MessageStatus{utility::decodeFromUtf8(event.status_info().message()), event.status_info().is_error()}.dispatch();
    break;

  case sourcetrail::EngineEvent::kErrorCount:
    // The errors themselves stay engine-side; the error view fetches them over its own RPC.
    MessageErrorCountUpdate{ErrorCountInfo{event.error_count().total(), event.error_count().fatal()}, {}}.dispatch();
    break;

  case sourcetrail::EngineEvent::kUnknownProgress:
    if(auto view = dialogView(DialogView::UseCase::GENERAL); view) {
      view->showUnknownProgressDialog(
          utility::decodeFromUtf8(event.unknown_progress().title()), utility::decodeFromUtf8(event.unknown_progress().message()));
    }
    break;

  case sourcetrail::EngineEvent::kProgress:
    if(auto view = dialogView(DialogView::UseCase::GENERAL); view) {
      view->showProgressDialog(utility::decodeFromUtf8(event.progress().title()),
                               utility::decodeFromUtf8(event.progress().message()),
                               event.progress().progress());
    }
    break;

  case sourcetrail::EngineEvent::kClearDialogs:
    if(auto view = dialogView(DialogView::UseCase::GENERAL); view) {
      view->clearDialogs();
    }
    break;

  case sourcetrail::EngineEvent::EVENT_NOT_SET:
  default:
    LOG_WARNING("Received an engine event with no payload.");
    break;
  }
}

}    // namespace client
