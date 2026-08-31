#pragma once
#include <functional>
#include <string>

#include "component/view/ViewLayout.h"

namespace shell {

/**
 * The layout the controllers reach when they want a panel brought forward.
 *
 * The widget GUI answered View::showView() by raising a dock widget. There are no docks here, but
 * the call still has to go somewhere: StatusController, ErrorController and CustomTrailController
 * all use it to say "the user needs to see this now", which in this design means selecting the
 * matching entry in the icon rail or opening the errors dock.
 *
 * A view constructed with a null layout would crash on the first such call, so every view-model
 * that hosts a controller which can raise itself passes one of these instead. Views are identified
 * by View::getName() -- "Status", "Errors", "custom trail" -- because that is the only handle
 * ViewLayout is given.
 */
class QmlViewLayout final : public ViewLayout {
public:
  /** Called with the raising view's name, on whatever thread the bus used. */
  using ShowHandler = std::function<void(const std::string& viewName)>;

  explicit QmlViewLayout(ShowHandler onShow);
  ~QmlViewLayout() override;

  void addView(View* view) override;
  void removeView(View* view) override;
  void showView(View* view) override;
  void hideView(View* view) override;
  void setViewEnabled(View* view, bool enabled) override;

private:
  ShowHandler mOnShow;
};

}    // namespace shell
