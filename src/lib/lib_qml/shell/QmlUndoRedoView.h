#pragma once
#include <functional>
#include <vector>

#include "component/view/UndoRedoView.h"
#include "shell/ShellSnapshot.h"

namespace shell {

/**
 * The UndoRedoView the controller pushes navigation state into.
 *
 * It draws nothing. UndoRedoController maintains the history stack and tells this whether the two
 * buttons are live and what the stack looks like; this folds all three calls into one snapshot and
 * hands it to a callback on whatever thread the bus used.
 *
 * The controller reports button state and history through separate calls, so a snapshot is
 * assembled from the last of each rather than from one call's arguments.
 */
class QmlUndoRedoView final : public UndoRedoView {
public:
  using SnapshotHandler = std::function<void(HistorySnapshot)>;

  explicit QmlUndoRedoView(SnapshotHandler onSnapshot);
  ~QmlUndoRedoView() override;

  void refreshView() override;

  void setRedoButtonEnabled(bool enabled) override;
  void setUndoButtonEnabled(bool enabled) override;
  void updateHistory(const std::vector<SearchMatch>& searchMatches, size_t currentIndex) override;

private:
  void publish();

  SnapshotHandler mOnSnapshot;
  HistorySnapshot mState;
};

}    // namespace shell
