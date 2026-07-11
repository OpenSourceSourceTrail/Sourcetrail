#pragma once
#include <gmock/gmock.h>

#include "SearchView.h"

struct MockedSearchView final : SearchView {
  MockedSearchView(ViewLayout* viewLayout) : SearchView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(std::wstring, getQuery, (), (const, override));

  MOCK_METHOD(void, setMatches, (const std::vector<SearchMatch>& matches), (override));

  MOCK_METHOD(void, setFocus, (), (override));
  MOCK_METHOD(void, findFulltext, (), (override));

  MOCK_METHOD(void, setAutocompletionList, (const std::vector<SearchMatch>&), (override));
};