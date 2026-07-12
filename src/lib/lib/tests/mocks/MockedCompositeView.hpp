#pragma once
#include <gmock/gmock.h>

#include "CompositeView.h"

struct MockedCompositeView final : CompositeView {
  explicit MockedCompositeView(ViewLayout* viewLayout)
      : CompositeView(viewLayout, CompositeDirection::DIRECTION_HORIZONTAL, "Mocked", Id{0}) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, addViewWidget, (View*), (override));

  MOCK_METHOD(void, showFocusIndicator, (bool), (override));
};