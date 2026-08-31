#include "shell/QmlStatusView.h"

#include <utility>

#include "utilityString.h"

namespace shell {

namespace {

// A long index run emits thousands of status lines; the panel is scrollback, not a transcript.
constexpr qsizetype MaxLines = 1000;

}    // namespace

QmlStatusView::QmlStatusView(ViewLayout* viewLayout, SnapshotHandler onSnapshot)
    : StatusView(viewLayout), mOnSnapshot(std::move(onSnapshot)) {}

QmlStatusView::~QmlStatusView() = default;

void QmlStatusView::refreshView() {}

void QmlStatusView::addStatus(const std::vector<Status>& status) {
  for(const Status& entry : status) {
    mLines.append(StatusLine{QString::fromStdString(utility::encodeToUtf8(entry.message)), entry.type == StatusType::Error});
  }
  if(mLines.size() > MaxLines) {
    mLines.remove(0, mLines.size() - MaxLines);
  }
  if(mOnSnapshot) {
    mOnSnapshot(mLines);
  }
}

void QmlStatusView::clear() {
  mLines.clear();
  if(mOnSnapshot) {
    mOnSnapshot(mLines);
  }
}

}    // namespace shell
