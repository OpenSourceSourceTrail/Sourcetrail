#include "qt/view/QtViewFactory.h"

#include "bookmark/ui/QtBookmarkButtonsView.h"
#include "bookmark/ui/QtBookmarkView.h"
#include "code/ui/QtCodeView.h"
#include "component/view/GraphViewStyle.h"
#include "custom_trail/ui/QtCustomTrailView.h"
#include "error/ui/QtErrorView.h"
#include "history/ui/QtUndoRedoView.h"
#include "qt/utility/utilityQt.h"
#include "qt/view/QtCompositeView.h"
#include "qt/view/QtDialogView.h"
#include "qt/view/QtGraphView.h"
#include "qt/view/QtGraphViewStyleImpl.h"
#include "qt/view/QtMainView.h"
#include "qt/view/QtTabbedView.h"
#include "refresh/ui/QtRefreshView.h"
#include "search/ui/QtScreenSearchView.h"
#include "search/ui/QtSearchView.h"
#include "status/ui/QtStatusBarView.h"
#include "status/ui/QtStatusView.h"
#include "tabs/ui/QtTabsView.h"
#include "tooltip/ui/QtTooltipView.h"

QtViewFactory::QtViewFactory() = default;

QtViewFactory::~QtViewFactory() = default;

std::shared_ptr<MainView> QtViewFactory::createMainView(StorageAccess* storageAccess) const {
  return std::make_shared<QtMainView>(this, storageAccess);
}

std::shared_ptr<CompositeView> QtViewFactory::createCompositeView(ViewLayout* viewLayout,
                                                                  CompositeView::CompositeDirection direction,
                                                                  const std::string& name,
                                                                  const Id tabId) const {
  return View::createAndAddToLayout<QtCompositeView>(viewLayout, direction, name, tabId);
}

std::shared_ptr<TabbedView> QtViewFactory::createTabbedView(ViewLayout* viewLayout, const std::string& name) const {
  return View::createAndAddToLayout<QtTabbedView>(viewLayout, name);
}

std::shared_ptr<BookmarkButtonsView> QtViewFactory::createBookmarkButtonsView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtBookmarkButtonsView>(viewLayout);
}

std::shared_ptr<BookmarkView> QtViewFactory::createBookmarkView(ViewLayout* viewLayout) const {
  return View::create<QtBookmarkView>(viewLayout);
}

std::shared_ptr<CodeView> QtViewFactory::createCodeView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtCodeView>(viewLayout);
}

std::shared_ptr<CustomTrailView> QtViewFactory::createCustomTrailView(ViewLayout* viewLayout) const {
  return View::create<QtCustomTrailView>(viewLayout);
}

std::shared_ptr<ErrorView> QtViewFactory::createErrorView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtErrorView>(viewLayout);
}

std::shared_ptr<StatusView> QtViewFactory::createStatusView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtStatusView>(viewLayout);
}

std::shared_ptr<GraphView> QtViewFactory::createGraphView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtGraphView>(viewLayout);
}

std::shared_ptr<RefreshView> QtViewFactory::createRefreshView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtRefreshView>(viewLayout);
}

std::shared_ptr<ScreenSearchView> QtViewFactory::createScreenSearchView(ViewLayout* viewLayout) const {
  return View::create<QtScreenSearchView>(viewLayout);
}

std::shared_ptr<SearchView> QtViewFactory::createSearchView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtSearchView>(viewLayout);
}

std::shared_ptr<StatusBarView> QtViewFactory::createStatusBarView(ViewLayout* viewLayout) const {
  return View::create<QtStatusBarView>(viewLayout);
}

std::shared_ptr<TabsView> QtViewFactory::createTabsView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtTabsView>(viewLayout);
}

std::shared_ptr<TooltipView> QtViewFactory::createTooltipView(ViewLayout* viewLayout) const {
  return View::create<QtTooltipView>(viewLayout);
}

std::shared_ptr<UndoRedoView> QtViewFactory::createUndoRedoView(ViewLayout* viewLayout) const {
  return View::createAndAddToLayout<QtUndoRedoView>(viewLayout);
}

std::shared_ptr<DialogView> QtViewFactory::createDialogView(ViewLayout* viewLayout,
                                                            DialogView::UseCase useCase,
                                                            StorageAccess* storageAccess) const {
  return std::make_shared<QtDialogView>(utility::getMainWindowforMainView(viewLayout), useCase, storageAccess);
}

std::shared_ptr<GraphViewStyleImpl> QtViewFactory::createGraphStyleImpl() const {
  return std::make_shared<QtGraphViewStyleImpl>();
}
