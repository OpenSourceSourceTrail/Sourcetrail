#include "EngineEventPublisher.h"

#include <memory>

#include "Application.h"
#include "ConvertEvents.h"
#include "EngineServiceImpl.h"

EngineDialogView::EngineDialogView(UseCase useCase, EngineServiceImpl* service)
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

void EngineDialogView::updateIndexingDialog(size_t startedFileCount,
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
  mService->broadcastEvent(proto::convert::toIndexingFinishedEvent(
      indexedFileCount, totalIndexedFileCount, completedFileCount, totalFileCount, time, errorInfo, interrupted, shallow));

  // Keeping the freshly indexed database is what the base class does headless, and asking the user
  // instead needs the Session bidi stream that engine.proto declares but nobody implements yet.
  return DATABASE_POLICY_KEEP;
}

EngineEventPublisher::EngineEventPublisher(EngineServiceImpl* service) : mService(service) {}

void EngineEventPublisher::installDialogViewFactory() const {
  EngineServiceImpl* service = mService;
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
