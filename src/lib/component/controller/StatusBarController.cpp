#include "StatusBarController.h"

#include "logging.h"
#include "StatusBarView.h"
#include "StorageAccess.h"

StatusBarController::StatusBarController(StorageAccess* storageAccess) : mStorageAccess(storageAccess) {}

StatusBarController::~StatusBarController() = default;

StatusBarView* StatusBarController::getView() {
  return Controller::getView<StatusBarView>();
}

void StatusBarController::clear() {
  getView()->setErrorCount(ErrorCountInfo{});
}

void StatusBarController::handleMessage(MessageErrorCountClear* /*message*/) {
  clear();
}

void StatusBarController::handleMessage(MessageErrorCountUpdate* message) {
  getView()->setErrorCount(message->errorCount);
}

void StatusBarController::handleMessage(MessageIndexingFinished* /*message*/) {
  getView()->setErrorCount(mStorageAccess->getErrorCount());
  getView()->hideIndexingProgress();
}

void StatusBarController::handleMessage(MessageIndexingStarted* /*message*/) {
  getView()->showIndexingProgress(0);
}

void StatusBarController::handleMessage(MessageIndexingStatus* message) {
  if(message->showProgress) {
    getView()->showIndexingProgress(message->progressPercent);
  } else {
    getView()->hideIndexingProgress();
  }
}

void StatusBarController::handleMessage(MessagePingReceived* message) {
  std::wstring status = L"No IDE connected";

  if(!message->ideName.empty()) {
    status = L"Connected to ";
    status += message->ideName;
  }

  getView()->showIdeStatus(status);
}

void StatusBarController::handleMessage(MessageRefresh* /*message*/) {
  getView()->setErrorCount(mStorageAccess->getErrorCount());
}

void StatusBarController::handleMessage(MessageStatus* message) {
  if(message->showInStatusBar) {
    setStatus(message->status(), message->isError, message->showLoader);
  }
}

void StatusBarController::setStatus(const std::wstring& status, bool isError, bool showLoader) {
  if(!status.empty()) {
    LOG_INFO(L"STATUS {}", status);
  }

  getView()->showMessage(status, isError, showLoader);
}
