#pragma once

#include <gmock/gmock.h>

#include "ViewFactory.h"

struct MockedViewFactory : public ViewFactory {
  using MainViewPtr = std::shared_ptr<MainView>;
  MOCK_METHOD(MainViewPtr, createMainView, (StorageAccess * storageAccess), (const, override));

  using CompositeViewPtr = std::shared_ptr<CompositeView>;
  MOCK_METHOD(CompositeViewPtr,
              createCompositeView,
              (ViewLayout * viewLayout, CompositeView::CompositeDirection direction, const std::string& name, const Id tabId),
              (const, override));

  using TabbedViewPtr = std::shared_ptr<TabbedView>;
  MOCK_METHOD(TabbedViewPtr, createTabbedView, (ViewLayout * viewLayout, const std::string& name), (const, override));

  using BookmarkButtonsViewPtr = std::shared_ptr<BookmarkButtonsView>;
  MOCK_METHOD(BookmarkButtonsViewPtr, createBookmarkButtonsView, (ViewLayout * viewLayout), (const, override));

  using BookmarkViewPtr = std::shared_ptr<BookmarkView>;
  MOCK_METHOD(BookmarkViewPtr, createBookmarkView, (ViewLayout * viewLayout), (const, override));

  using CodeViewPtr = std::shared_ptr<CodeView>;
  MOCK_METHOD(CodeViewPtr, createCodeView, (ViewLayout * viewLayout), (const, override));

  using CustomTrailViewPtr = std::shared_ptr<CustomTrailView>;
  MOCK_METHOD(CustomTrailViewPtr, createCustomTrailView, (ViewLayout * viewLayout), (const, override));

  using ErrorViewPtr = std::shared_ptr<ErrorView>;
  MOCK_METHOD(ErrorViewPtr, createErrorView, (ViewLayout * viewLayout), (const, override));

  using GraphViewPtr = std::shared_ptr<GraphView>;
  MOCK_METHOD(GraphViewPtr, createGraphView, (ViewLayout * viewLayout), (const, override));

  using RefreshViewPtr = std::shared_ptr<RefreshView>;
  MOCK_METHOD(RefreshViewPtr, createRefreshView, (ViewLayout * viewLayout), (const, override));

  using ScreenSearchViewPtr = std::shared_ptr<ScreenSearchView>;
  MOCK_METHOD(ScreenSearchViewPtr, createScreenSearchView, (ViewLayout * viewLayout), (const, override));

  using SearchViewPtr = std::shared_ptr<SearchView>;
  MOCK_METHOD(SearchViewPtr, createSearchView, (ViewLayout * viewLayout), (const, override));

  using StatusBarViewPtr = std::shared_ptr<StatusBarView>;
  MOCK_METHOD(StatusBarViewPtr, createStatusBarView, (ViewLayout * viewLayout), (const, override));

  using StatusViewPtr = std::shared_ptr<StatusView>;
  MOCK_METHOD(StatusViewPtr, createStatusView, (ViewLayout * viewLayout), (const, override));

  using TabsViewPtr = std::shared_ptr<TabsView>;
  MOCK_METHOD(TabsViewPtr, createTabsView, (ViewLayout * viewLayout), (const, override));

  using TooltipViewPtr = std::shared_ptr<TooltipView>;
  MOCK_METHOD(TooltipViewPtr, createTooltipView, (ViewLayout * viewLayout), (const, override));

  using UndoRedoViewPtr = std::shared_ptr<UndoRedoView>;
  MOCK_METHOD(UndoRedoViewPtr, createUndoRedoView, (ViewLayout * viewLayout), (const, override));

  using DialogViewPtr = std::shared_ptr<DialogView>;
  MOCK_METHOD(DialogViewPtr,
              createDialogView,
              (ViewLayout * viewLayout, DialogView::UseCase useCase, StorageAccess* storageAccess),
              (const, override));

  using GraphViewStyleImplPtr = std::shared_ptr<GraphViewStyleImpl>;
  MOCK_METHOD(GraphViewStyleImplPtr, createGraphStyleImpl, (), (const, override));
};
