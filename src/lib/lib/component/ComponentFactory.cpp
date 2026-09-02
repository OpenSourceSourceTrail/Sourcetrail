#include "component/ComponentFactory.h"

#include <memory>

#include "activation/logic/ActivationController.h"
#include "app/Application.h"
#include "bookmark/logic/BookmarkController.h"
#include "bookmark/logic/BookmarkView.h"
#include "code/logic/CodeController.h"
#include "code/logic/CodeView.h"
#include "component/Component.h"
#include "component/view/ViewFactory.h"
#include "custom_trail/logic/CustomTrailController.h"
#include "custom_trail/logic/CustomTrailView.h"
#include "error/logic/ErrorController.h"
#include "error/logic/ErrorView.h"
#include "graph/logic/GraphController.h"
#include "graph/logic/GraphView.h"
#include "history/logic/UndoRedoController.h"
#include "history/logic/UndoRedoView.h"
#include "refresh/logic/RefreshController.h"
#include "refresh/logic/RefreshView.h"
#include "search/logic/ScreenSearchController.h"
#include "search/logic/ScreenSearchView.h"
#include "search/logic/SearchController.h"
#include "search/logic/SearchView.h"
#include "status/logic/StatusBarController.h"
#include "status/logic/StatusBarView.h"
#include "status/logic/StatusController.h"
#include "status/logic/StatusView.h"
#include "tabs/logic/TabsController.h"
#include "tabs/logic/TabsView.h"
#include "tooltip/logic/TooltipController.h"
#include "tooltip/logic/TooltipView.h"

ComponentFactory::ComponentFactory(const ViewFactory* viewFactory, StorageAccess* storageAccess)
    : m_viewFactory(viewFactory), m_storageAccess(storageAccess) {}

const ViewFactory* ComponentFactory::getViewFactory() const {
  return m_viewFactory;
}

StorageAccess* ComponentFactory::getStorageAccess() const {
  return m_storageAccess;
}

std::shared_ptr<Component> ComponentFactory::createActivationComponent() {
  std::shared_ptr<Controller> controller = std::make_shared<ActivationController>(m_storageAccess);

  return std::make_shared<Component>(nullptr, controller);
}

std::shared_ptr<Component> ComponentFactory::createBookmarkComponent(ViewLayout* viewLayout) {
  std::shared_ptr<BookmarkView> view = m_viewFactory->createBookmarkView(viewLayout);
  std::shared_ptr<BookmarkController> controller = std::make_shared<BookmarkController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createCodeComponent(ViewLayout* viewLayout) {
  std::shared_ptr<CodeView> view = m_viewFactory->createCodeView(viewLayout);
  std::shared_ptr<CodeController> controller = std::make_shared<CodeController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createCustomTrailComponent(ViewLayout* viewLayout) {
  std::shared_ptr<CustomTrailView> view = m_viewFactory->createCustomTrailView(viewLayout);
  std::shared_ptr<CustomTrailController> controller = std::make_shared<CustomTrailController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createErrorComponent(ViewLayout* viewLayout) {
  std::shared_ptr<ErrorView> view = m_viewFactory->createErrorView(viewLayout);
  std::shared_ptr<ErrorController> controller = std::make_shared<ErrorController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createGraphComponent(ViewLayout* viewLayout) {
  std::shared_ptr<View> view = m_viewFactory->createGraphView(viewLayout);
  std::shared_ptr<GraphController> controller = std::make_shared<GraphController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createRefreshComponent(ViewLayout* viewLayout) {
  std::shared_ptr<View> view = m_viewFactory->createRefreshView(viewLayout);
  std::shared_ptr<RefreshController> controller = std::make_shared<RefreshController>();

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createScreenSearchComponent(ViewLayout* viewLayout) {
  std::shared_ptr<ScreenSearchView> view = m_viewFactory->createScreenSearchView(viewLayout);
  std::shared_ptr<ScreenSearchController> controller = std::make_shared<ScreenSearchController>();

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createSearchComponent(ViewLayout* viewLayout) {
  std::shared_ptr<SearchView> view = m_viewFactory->createSearchView(viewLayout);
  std::shared_ptr<SearchController> controller = std::make_shared<SearchController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createStatusBarComponent(ViewLayout* viewLayout) {
  std::shared_ptr<StatusBarView> view = m_viewFactory->createStatusBarView(viewLayout);
  std::shared_ptr<StatusBarController> controller = std::make_shared<StatusBarController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createStatusComponent(ViewLayout* viewLayout) {
  std::shared_ptr<StatusView> view = m_viewFactory->createStatusView(viewLayout);
  std::shared_ptr<StatusController> controller = std::make_shared<StatusController>();

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createTabsComponent(ViewLayout* viewLayout, ScreenSearchSender* screenSearchSender) {
  std::shared_ptr<TabsView> view = m_viewFactory->createTabsView(viewLayout);
  std::shared_ptr<Controller> controller = std::make_shared<TabsController>(
      viewLayout, m_viewFactory, m_storageAccess, screenSearchSender, [] { return Application::getInstance()->isProjectLoaded(); });

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createTooltipComponent(ViewLayout* viewLayout) {
  std::shared_ptr<TooltipView> view = m_viewFactory->createTooltipView(viewLayout);
  std::shared_ptr<Controller> controller = std::make_shared<TooltipController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}

std::shared_ptr<Component> ComponentFactory::createUndoRedoComponent(ViewLayout* viewLayout) {
  std::shared_ptr<UndoRedoView> view = m_viewFactory->createUndoRedoView(viewLayout);
  std::shared_ptr<UndoRedoController> controller = std::make_shared<UndoRedoController>(m_storageAccess);

  return std::make_shared<Component>(view, controller);
}
