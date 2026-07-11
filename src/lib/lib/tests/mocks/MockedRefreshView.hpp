#pragma once
#include <gmock/gmock.h>

#include "RefreshView.h"

struct MockedRefreshView final : RefreshView {
  explicit MockedRefreshView(ViewLayout* viewLayout) : RefreshView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));
};