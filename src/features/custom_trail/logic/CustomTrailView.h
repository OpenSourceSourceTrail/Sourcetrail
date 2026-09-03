#ifndef CUSTOM_TRAIL_VIEW_H
#define CUSTOM_TRAIL_VIEW_H

#include "component/view/View.h"
#include "search/domain/SearchMatch.h"

class CustomTrailView : public View {
public:
  CustomTrailView(ViewLayout*) : View(nullptr) {}

  virtual std::string getName() const {
    return "custom trail";
  }

  virtual void clearView() = 0;
  virtual void setAvailableNodeAndEdgeTypes(NodeKindMask nodeTypes, Edge::TypeMask edgeTypes) = 0;

  virtual void showView() = 0;
  virtual void hideView() = 0;
  virtual void showAutocompletions(const std::vector<SearchMatch>& autocompletions, bool from) = 0;
};

#endif    // CUSTOM_TRAIL_VIEW_H
