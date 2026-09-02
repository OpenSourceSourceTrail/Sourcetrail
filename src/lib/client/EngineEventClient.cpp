#include "EngineEventClient.h"

#include <chrono>
#include <memory>

#include "app/Application.h"
#include "ConvertEvents.h"
#include "EngineCall.h"
#include "EngineChannel.h"
#include "error/domain/ErrorCountInfo.h"
#include "error/messages/MessageErrorCountUpdate.h"
#include "HttpClient.h"
#include "logging.h"
#include "ProtoJson.h"
#include "status/messages/MessageStatus.h"
#include "type/indexing/MessageIndexingFinished.h"
#include "type/indexing/MessageIndexingStarted.h"
#include "type/indexing/MessageIndexingStatus.h"
#include "utilityString.h"

namespace client {

namespace {

constexpr std::chrono::milliseconds ReconnectDelay{500};

std::shared_ptr<DialogView> dialogView(DialogView::UseCase useCase) {
  Application* application = Application::getInstance().get();
  return application != nullptr ? application->getDialogView(useCase) : nullptr;
}

/** Posts one dialog answer back to the engine, which is blocked waiting for it. */
void postDialogAnswer(EngineChannel* channel, uint64_t requestId, int selectedOption) {
  sourcetrail::DialogResponse response;
  response.set_request_id(requestId);
  response.set_selected_option(selectedOption);

  std::ignore = callVoid(
      channel, "answerDialog", "POST", "/api/v1/dialogs/" + std::to_string(requestId), proto::json::toJson(response));
}

/**
 * Raises the question the engine is blocked on, then answers it.
 *
 * The dialog blocks until the user closes it, so it runs on a thread of its own: doing it on the
 * reader thread would stop us consuming the stream, and the engine's broadcast would then back up
 * behind its own listener lock and stall indexing.
 * ponytail: detached thread; give the reader a work queue if more blocking questions appear.
 */
void answerDialog(EngineChannel* channel, const sourcetrail::DialogRequest& request) {
  if(request.request_case() != sourcetrail::DialogRequest::kFinishedIndexing) {
    LOG_WARNING("Received a dialog request this client cannot answer; leaving it to time out.");
    return;
  }

  auto view = dialogView(DialogView::UseCase::INDEXING);
  if(!view) {
    // Headless: let the engine fall back to its own default rather than wait out the timeout.
    postDialogAnswer(channel, request.request_id(), DATABASE_POLICY_KEEP);
    return;
  }

  const sourcetrail::IndexingFinishedEvent& summary = request.finished_indexing().summary();
  std::thread([channel,
               view,
               requestId = request.request_id(),
               indexed = summary.indexed_file_count(),
               totalIndexed = summary.total_indexed_file_count(),
               completed = summary.completed_file_count(),
               total = summary.total_file_count(),
               seconds = summary.time_seconds(),
               errors = ErrorCountInfo{summary.error_total(), summary.error_fatal()},
               interrupted = summary.interrupted(),
               shallow = summary.shallow()]() {
    const DatabasePolicy policy = view->finishedIndexingDialog(
        indexed, totalIndexed, completed, total, seconds, errors, interrupted, shallow);
    postDialogAnswer(channel, requestId, static_cast<int>(policy));
  }).detach();
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
    const std::lock_guard<std::mutex> lock(mSourceMutex);
    if(mSource) {
      mSource->cancel();
    }
  }
  mThread.join();
}

void EngineEventClient::run() {
  while(!mStopping) {
    if(mChannel == nullptr || mChannel->getPort() == 0) {
      std::this_thread::sleep_for(ReconnectDelay);
      continue;
    }

    // Rebuilt each attempt: after a respawn the engine answers on a new port with a new token.
    auto source = std::make_shared<http::EventSource>("127.0.0.1", mChannel->getPort(), mChannel->getAuthToken());
    {
      const std::lock_guard<std::mutex> lock(mSourceMutex);
      if(mStopping) {
        return;
      }
      mSource = source;
    }

    source->run("/api/v1/events", [this](const std::string& name, const std::string& data) { handleFrame(name, data); });

    {
      const std::lock_guard<std::mutex> lock(mSourceMutex);
      mSource.reset();
    }

    if(!mStopping) {
      // The engine went away; the supervisor is restarting it and the channel will point at the new
      // port once the handshake lands.
      std::this_thread::sleep_for(ReconnectDelay);
    }
  }
}

void EngineEventClient::handleFrame(const std::string& name, const std::string& data) {
  // Dialog questions block the engine until answered, so they are not ordinary one-way events.
  if(name == "dialog") {
    sourcetrail::DialogRequest request;
    if(!proto::json::fromJson(data, request)) {
      LOG_WARNING("Received a malformed dialog request from the engine.");
      return;
    }
    answerDialog(mChannel, request);
    return;
  }

  sourcetrail::EngineEvent event;
  if(!proto::json::fromJson(data, event)) {
    LOG_WARNING("Received a malformed engine event: " + name);
    return;
  }
  apply(event);
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
    // The engine's own MessageIndexingStatus never crosses the boundary; DialogView::updateIndexingDialog
    // republishes it locally from the counts that did.
    break;
  }

  case sourcetrail::EngineEvent::kIndexingFinished:
    // The report dialog is no longer raised here. The engine asks for it separately, over the dialog
    // channel, because it blocks on the answer -- see answerDialog below.
    MessageIndexingStatus{false}.dispatch();
    MessageIndexingFinished{}.dispatch();
    break;

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
