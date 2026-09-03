#include "refresh/logic/RefreshController.h"

#include "refresh/logic/RefreshView.h"

RefreshController::RefreshController() {}

RefreshController::~RefreshController() {}

void RefreshController::clear() {}

RefreshView* RefreshController::getView() {
  return Controller::getView<RefreshView>();
}
