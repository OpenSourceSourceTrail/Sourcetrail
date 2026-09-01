#include "EngineEventPublisher.h"

#include <chrono>
#include <memory>
#include <optional>

#include "app/Application.h"
#include "ConvertEvents.h"
#include "engine.pb.h"
#include "EngineHttpService.h"

namespace {

// Long enough for a user to read the indexing report and decide; if nobody is there to answer, the
// engine falls back to keeping the database rather than blocking indefinitely.
constexpr std::chrono::milliseconds FinishedIndexingTimeout{10 * 60 * 1000};

}    // namespace

EngineDialogView::EngineDialogView(UseCase useCase, EngineHttpService* service)
    : DialogView(useCase, nullptr), mService(service) {}

void EngineDialogView::clearDialogs() {
  mService->broadcastEvent(proto::convert::toClearDialogsEvent());
}

void EngineDialogView::showUnknownProgressDialog(const std::wstring& title, const std::wstring& message) {
  mService->broadcastEvent(proto::convert::toUnknownProgressEvent(title, message));
}

void EngineDialogView::hideUnknownProgressDialog() {
  mService->broadcastEvent(proto::convert::toClearDialogsEvent());
}

void EngineDialogView::showProgressDialog(const std::wstring& title, const std::wstring& message, size_t progress) {
  mService->broadcastEvent(proto::convert::toProgressEvent(title, message, progress));
}

void EngineDialogView::hideProgressDialog() {
  mService->broadcastEvent(proto::convert::toClearDialogsEvent());
}

void EngineDialogView::doUpdateIndexingDialog(size_t startedFileCount,
                                              size_t finishedFileCount,
                                              size_t totalFileCount,
                                              const std::vector<FilePath>& sourcePaths) {
  mService->broadcastEvent(
      proto::convert::toIndexingProgressEvent(startedFileCount, finishedFileCount, totalFileCount, sourcePaths));
}

void EngineDialogView::updateCustomIndexingDialog(size_t startedFileCount,
                                                  size_t finishedFileCount,
                                                  size_t totalFileCount,
                                                  const std::vector<FilePath>& sourcePaths) {
  // The client tells the two apart by the dialog title, which it owns; the payload is identical.
  updateIndexingDialog(startedFileCount, finishedFileCount, totalFileCount, sourcePaths);
}

DatabasePolicy EngineDialogView::finishedIndexingDialog(size_t indexedFileCount,
                                                        size_t totalIndexedFileCount,
                                                        size_t completedFileCount,
                                                        size_t totalFileCount,
                                                        float time,
                                                        ErrorCountInfo errorInfo,
                                                        bool interrupted,
                                                        bool shallow) {
  const sourcetrail::EngineEvent summary = proto::convert::toIndexingFinishedEvent(
      indexedFileCount, totalIndexedFileCount, completedFileCount, totalFileCount, time, errorInfo, interrupted, shallow);
  mService->broadcastEvent(summary);

  // The answer decides whether the freshly built temporary database replaces the project's, so it
  // has to come from the user. Falling back to KEEP -- what the headless base class does -- covers
  // the cases where nobody can answer: no client attached, or none answered in time.
  sourcetrail::DialogRequest request;
  *request.mutable_finished_indexing()->mutable_summary() = summary.indexing_finished();

  const std::optional<int> answer = mService->askDialog(request, FinishedIndexingTimeout);
  if(!answer.has_value()) {
    return DATABASE_POLICY_KEEP;
  }
  return static_cast<DatabasePolicy>(*answer);
}

EngineEventPublisher::EngineEventPublisher(EngineHttpService* service) : mService(service) {}

void EngineEventPublisher::installDialogViewFactory() const {
  EngineHttpService* service = mService;
  Application::getInstance()->setDialogViewFactory(
      [service](DialogView::UseCase useCase) { return std::make_shared<EngineDialogView>(useCase, service); });
}

void EngineEventPublisher::handleMessage(MessageIndexingStarted* /*message*/) {
  mService->broadcastEvent(proto::convert::toIndexingStartedEvent());
}

void EngineEventPublisher::handleMessage(MessageStatus* message) {
  mService->broadcastEvent(proto::convert::toStatusInfoEvent(message->status(), message->isError));
}

void EngineEventPublisher::handleMessage(MessageErrorCountUpdate* message) {
  mService->broadcastEvent(proto::convert::toErrorCountEvent(message->errorCount.total, message->errorCount.fatal));
}
