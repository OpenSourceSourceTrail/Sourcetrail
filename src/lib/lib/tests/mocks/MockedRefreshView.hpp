#pragma once
#include <gmock/gmock.h>

#include "component/view/RefreshView.h"

struct MockedRefreshView final : RefreshView {
  explicit MockedRefreshView(ViewLayout* viewLayout) : RefreshView(viewLayout) {}

  MOCK_METHOD(void, refreshView, (), (override));
};