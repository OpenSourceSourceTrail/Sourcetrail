#pragma once
#include <gmock/gmock.h>

#include "component/view/TabbedView.h"

struct MockedTabbedView final : TabbedView {
  MockedTabbedView(ViewLayout* viewLayout) : TabbedView(viewLayout, "Mocked") {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));
  MOCK_METHOD(void, addViewWidget, (View*), (override));
};