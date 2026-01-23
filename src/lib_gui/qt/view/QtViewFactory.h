#pragma once
#include <memory>

#include "ChatView.hpp"
#include "ViewFactory.h"


class QtViewFactory final : public ViewFactory {
public:
  QtViewFactory();

  ~QtViewFactory() override;

  std::shared_ptr<MainView> createMainView(StorageAccess* storageAccess) const override;
  std::shared_ptr<CompositeView> createCompositeView(ViewLayout* viewLayout,
                                                     CompositeView::CompositeDirection direction,
                                                     const std::string& name,
                                                     const Id tabId) const override;
  std::shared_ptr<TabbedView> createTabbedView(ViewLayout* viewLayout, const std::string& name) const override;

  std::shared_ptr<BookmarkButtonsView> createBookmarkButtonsView(ViewLayout* viewLayout) const override;
  std::shared_ptr<BookmarkView> createBookmarkView(ViewLayout* viewLayout) const override;
  std::shared_ptr<CodeView> createCodeView(ViewLayout* viewLayout) const override;
  std::shared_ptr<CustomTrailView> createCustomTrailView(ViewLayout* viewLayout) const override;
  std::shared_ptr<ErrorView> createErrorView(ViewLayout* viewLayout) const override;
  std::shared_ptr<GraphView> createGraphView(ViewLayout* viewLayout) const override;
  std::shared_ptr<RefreshView> createRefreshView(ViewLayout* viewLayout) const override;
  std::shared_ptr<ScreenSearchView> createScreenSearchView(ViewLayout* viewLayout) const override;
  std::shared_ptr<SearchView> createSearchView(ViewLayout* viewLayout) const override;
  std::shared_ptr<StatusBarView> createStatusBarView(ViewLayout* viewLayout) const override;
  std::shared_ptr<StatusView> createStatusView(ViewLayout* viewLayout) const override;
  std::shared_ptr<TabsView> createTabsView(ViewLayout* viewLayout) const override;
  std::shared_ptr<TooltipView> createTooltipView(ViewLayout* viewLayout) const override;
  std::shared_ptr<UndoRedoView> createUndoRedoView(ViewLayout* viewLayout) const override;

  std::shared_ptr<DialogView> createDialogView(ViewLayout* viewLayout,
                                               DialogView::UseCase useCase,
                                               StorageAccess* storageAccess) const override;

  std::shared_ptr<GraphViewStyleImpl> createGraphStyleImpl() const override;

  std::shared_ptr<ChatView> createChatView(ViewLayout* viewLayout, std::shared_ptr<ChatModel> model) const;
};