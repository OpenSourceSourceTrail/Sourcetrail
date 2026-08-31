#include "shell/QmlUndoRedoView.h"

#include <utility>

#include "data/search/SearchMatch.h"
#include "utilityString.h"

namespace shell {

namespace {

QString toQt(const std::wstring& text) {
  return QString::fromStdString(utility::encodeToUtf8(text));
}

}    // namespace

QmlUndoRedoView::QmlUndoRedoView(SnapshotHandler onSnapshot) : UndoRedoView(nullptr), mOnSnapshot(std::move(onSnapshot)) {}

QmlUndoRedoView::~QmlUndoRedoView() = default;

void QmlUndoRedoView::refreshView() {}

void QmlUndoRedoView::setRedoButtonEnabled(bool enabled) {
  mState.canRedo = enabled;
  publish();
}

void QmlUndoRedoView::setUndoButtonEnabled(bool enabled) {
  mState.canUndo = enabled;
  publish();
}

void QmlUndoRedoView::updateHistory(const std::vector<SearchMatch>& searchMatches, size_t currentIndex) {
  mState.items.clear();
  mState.items.reserve(static_cast<qsizetype>(searchMatches.size()));

  for(size_t index = 0; index < searchMatches.size(); ++index) {
    const SearchMatch& match = searchMatches[index];
    mState.items.append(
        HistoryItem{toQt(match.name), toQt(match.typeName), static_cast<int>(match.nodeType.getKind()), index == currentIndex});
  }
  mState.currentIndex = static_cast<int>(currentIndex);
  publish();
}

void QmlUndoRedoView::publish() {
  if(mOnSnapshot) {
    mOnSnapshot(mState);
  }
}

}    // namespace shell
