#pragma once
#include <gmock/gmock.h>

#include "GraphView.h"

struct MockedGraphView final : GraphView {
  explicit MockedGraphView(ViewLayout* viewLayout) : GraphView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));
  MOCK_METHOD(void, refreshView, (), (override));

  using DummyNodeVec = std::vector<std::shared_ptr<DummyNode>>;
  using DummyEdgeVec = std::vector<std::shared_ptr<DummyEdge>>;
  MOCK_METHOD(void, rebuildGraph, (std::shared_ptr<Graph>, const DummyNodeVec&, const DummyEdgeVec&, const GraphParams), (override));
  MOCK_METHOD(void, clear, (), (override));

  MOCK_METHOD(void, coFocusTokenIds, (const std::vector<Id>&), (override));
  MOCK_METHOD(void, deCoFocusTokenIds, (const std::vector<Id>&), (override));

  MOCK_METHOD(void, resizeView, (), (override));

  MOCK_METHOD(QVector2D, getViewSize, (), (const, override));
  MOCK_METHOD(GroupType, getGrouping, (), (const, override));

  MOCK_METHOD(void, scrollToValues, (int, int), (override));

  MOCK_METHOD(void, activateEdge, (Id), (override));

  MOCK_METHOD(void, setNavigationFocus, (bool), (override));
  MOCK_METHOD(bool, hasNavigationFocus, (), (const, override));

  MOCK_METHOD(bool, isVisible, (), (const, override));

  MOCK_METHOD(void, findMatches, (ScreenSearchSender*, const std::wstring&), (override));

  MOCK_METHOD(void, activateMatch, (size_t), (override));

  MOCK_METHOD(void, deactivateMatch, (size_t), (override));

  MOCK_METHOD(void, clearMatches, (), (override));
};