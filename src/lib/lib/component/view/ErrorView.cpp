#include "ErrorView.h"

ErrorView::ErrorView(ViewLayout* pViewLayout) : View(pViewLayout) {}

ErrorView::~ErrorView() = default;

std::string ErrorView::getName() const {
  return "Errors";
}
