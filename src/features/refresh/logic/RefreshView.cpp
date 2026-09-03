#include "refresh/logic/RefreshView.h"

#include "refresh/logic/RefreshController.h"

RefreshView::RefreshView(ViewLayout* viewLayout) : View(viewLayout) {}

RefreshView::~RefreshView() = default;

std::string RefreshView::getName() const {
  return "RefreshView";
}

RefreshController* RefreshView::getController() {
  return View::getController<RefreshController>();
}
