#include "search/logic/SearchView.h"

#include "search/logic/SearchController.h"

SearchView::SearchView(ViewLayout* viewLayout) : View(viewLayout) {}

SearchView::~SearchView() {}

std::string SearchView::getName() const {
  return "Search";
}

SearchController* SearchView::getController() {
  return View::getController<SearchController>();
}
