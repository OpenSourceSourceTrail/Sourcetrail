#pragma once
#include <gmock/gmock.h>

#include "View.h"

struct MockedView final : View {
  explicit MockedView(ViewLayout* layout) : View(layout) {}
  MOCK_METHOD(std::string, getName, (), (const, override));

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));
};