#pragma once
#include <gmock/gmock.h>

#include "component/view/TooltipView.h"

struct MockedTooltipView : TooltipView {
  explicit MockedTooltipView(ViewLayout* viewLayout) : TooltipView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, showTooltip, (const TooltipInfo&, const View*), (override));
  MOCK_METHOD(void, hideTooltip, (bool), (override));
  MOCK_METHOD(bool, tooltipVisible, (), (const, override));
};
