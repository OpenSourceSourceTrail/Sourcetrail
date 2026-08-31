#pragma once
#include <functional>

#include "component/view/StatusBarView.h"
#include "shell/ShellSnapshot.h"

namespace shell {

/**
 * The StatusBarView the controller pushes the bottom bar's contents into.
 *
 * StatusBarController reports message, error count, IDE connection and indexing progress through
 * four separate calls, so the snapshot carries the last of each rather than one call's arguments.
 */
class QmlStatusBarView final : public StatusBarView {
public:
  using SnapshotHandler = std::function<void(StatusSnapshot)>;

  explicit QmlStatusBarView(ViewLayout* viewLayout, SnapshotHandler onSnapshot);
  ~QmlStatusBarView() override;

  void refreshView() override;

  void showMessage(const std::wstring& message, bool isError, bool showLoader) override;
  void setErrorCount(ErrorCountInfo errorCount) override;
  void showIdeStatus(const std::wstring& message) override;
  void showIndexingProgress(size_t progressPercent) override;
  void hideIndexingProgress() override;

private:
  void publish();

  SnapshotHandler mOnSnapshot;
  StatusSnapshot mState;
};

}    // namespace shell
