#include "status/logic/StatusBarView.h"

#include "status/logic/StatusBarController.h"

StatusBarView::StatusBarView(ViewLayout* viewLayout) : View(viewLayout) {}

StatusBarView::~StatusBarView() = default;

std::string StatusBarView::getName() const {
  return "StatusBarView";
}

StatusBarController* StatusBarView::getStatusBarController() {
  return View::getController<StatusBarController>();
}
