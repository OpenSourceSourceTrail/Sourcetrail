#pragma once
#include <functional>
#include <vector>

#include <QList>

#include "component/view/StatusView.h"
#include "Status.h"

namespace shell {

/** One line in the status log the "Status" panel shows. */
struct StatusLine final {
  QString message;
  bool isError = false;
};

/**
 * The StatusView the controller appends log lines to.
 *
 * Distinct from QmlStatusBarView: the bar shows the newest message, this is the scrollback behind
 * it. StatusController raises this panel through View::showView(), which is why it is constructed
 * with a real ViewLayout rather than a null one.
 */
class QmlStatusView final : public StatusView {
public:
  using SnapshotHandler = std::function<void(QList<StatusLine>)>;

  explicit QmlStatusView(ViewLayout* viewLayout, SnapshotHandler onSnapshot);
  ~QmlStatusView() override;

  void refreshView() override;

  void addStatus(const std::vector<Status>& status) override;
  void clear() override;

private:
  SnapshotHandler mOnSnapshot;
  QList<StatusLine> mLines;
};

}    // namespace shell
