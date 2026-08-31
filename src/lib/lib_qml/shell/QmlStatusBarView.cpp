#include "shell/QmlStatusBarView.h"

#include <utility>

#include "utilityString.h"

namespace shell {

namespace {

QString toQt(const std::wstring& text) {
  return QString::fromStdString(utility::encodeToUtf8(text));
}

}    // namespace

QmlStatusBarView::QmlStatusBarView(ViewLayout* viewLayout, SnapshotHandler onSnapshot)
    : StatusBarView(viewLayout), mOnSnapshot(std::move(onSnapshot)) {}

QmlStatusBarView::~QmlStatusBarView() = default;

void QmlStatusBarView::refreshView() {}

void QmlStatusBarView::showMessage(const std::wstring& message, bool isError, bool showLoader) {
  mState.message = toQt(message);
  mState.isError = isError;
  mState.showLoader = showLoader;
  publish();
}

void QmlStatusBarView::setErrorCount(ErrorCountInfo errorCount) {
  mState.errorTotal = static_cast<int>(errorCount.total);
  mState.errorFatal = static_cast<int>(errorCount.fatal);
  publish();
}

void QmlStatusBarView::showIdeStatus(const std::wstring& message) {
  mState.ideStatus = toQt(message);
  publish();
}

void QmlStatusBarView::showIndexingProgress(size_t progressPercent) {
  mState.indexingPercent = static_cast<int>(progressPercent);
  publish();
}

void QmlStatusBarView::hideIndexingProgress() {
  // -1 rather than 0: the bar distinguishes "not indexing" from "indexing, 0% done".
  mState.indexingPercent = -1;
  publish();
}

void QmlStatusBarView::publish() {
  if(mOnSnapshot) {
    mOnSnapshot(mState);
  }
}

}    // namespace shell
