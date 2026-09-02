#pragma once
#include <gmock/gmock.h>

#include "component/view/TabsView.h"

struct MockedTabsView final : TabsView {
  explicit MockedTabsView(ViewLayout* viewLayout) : TabsView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, clear, (), (override));
  MOCK_METHOD(void, openTab, (bool, const SearchMatch&), (override));
  MOCK_METHOD(void, closeTab, (), (override));
  MOCK_METHOD(void, destroyTab, (Id), (override));
  MOCK_METHOD(void, selectTab, (bool), (override));
  MOCK_METHOD(void, updateTab, (Id, const std::vector<SearchMatch>&), (override));
};
