#pragma once
#include <gmock/gmock.h>

#include "component/view/CustomTrailView.h"

struct MockedCustomTrailView : CustomTrailView {
  explicit MockedCustomTrailView(ViewLayout* viewLayout) : CustomTrailView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, clearView, (), (override));
  MOCK_METHOD(void, setAvailableNodeAndEdgeTypes, (NodeKindMask, Edge::TypeMask), (override));
  MOCK_METHOD(void, showView, (), (override));
  MOCK_METHOD(void, hideView, (), (override));
  MOCK_METHOD(void, showAutocompletions, (const std::vector<SearchMatch>&, bool), (override));
};
