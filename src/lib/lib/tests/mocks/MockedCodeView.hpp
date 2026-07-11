#pragma once
#include <gmock/gmock.h>

#include "CodeView.h"

struct MockedCodeView final : CodeView {
  explicit MockedCodeView(ViewLayout* viewLayout) : CodeView(viewLayout) {}

  MOCK_METHOD(void, createWidgetWrapper, (), (override));

  MOCK_METHOD(void, refreshView, (), (override));

  MOCK_METHOD(void, clear, (), (override));

  MOCK_METHOD(void, showSnippets, (const std::vector<CodeFileParams>&, const CodeParams&, const CodeScrollParams&), (override));

  MOCK_METHOD(void, showSingleFile, (const CodeFileParams&, const CodeParams&, const CodeScrollParams&), (override));

  MOCK_METHOD(void, updateSourceLocations, (const std::vector<CodeFileParams>&), (override));

  MOCK_METHOD(void, scrollTo, (const CodeScrollParams&, bool), (override));

  MOCK_METHOD(bool, showsErrors, (), (const, override));

  MOCK_METHOD(void, coFocusTokenIds, (const std::vector<Id>&), (override));
  MOCK_METHOD(void, deCoFocusTokenIds, (), (override));

  MOCK_METHOD(bool, isInListMode, (), (const, override));
  MOCK_METHOD(void, setMode, (bool), (override));

  MOCK_METHOD(bool, hasSingleFileCached, (const FilePath&), (const, override));

  MOCK_METHOD(void, setNavigationFocus, (bool), (override));
  MOCK_METHOD(bool, hasNavigationFocus, (), (const, override));

  MOCK_METHOD(bool, isVisible, (), (const, override));

  MOCK_METHOD(void, findMatches, (ScreenSearchSender*, const std::wstring&), (override));

  MOCK_METHOD(void, activateMatch, (size_t), (override));

  MOCK_METHOD(void, deactivateMatch, (size_t), (override));

  MOCK_METHOD(void, clearMatches, (), (override));
};