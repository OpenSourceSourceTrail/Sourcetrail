#pragma once
// STL
#include <memory>
// internal
#include "StorageAccess.h"

class Component;
class ViewFactory;
class ViewLayout;
class ScreenSearchSender;
class ChatModel;

namespace sourcetrail::lib_llm::services {
class LlmCoordinator;
}

class ComponentFactory {
public:
  ComponentFactory(const ViewFactory* viewFactory, StorageAccess* storageAccess);

  [[nodiscard]] const ViewFactory* getViewFactory() const;
  [[nodiscard]] StorageAccess* getStorageAccess() const;

  std::shared_ptr<Component> createActivationComponent();
  std::shared_ptr<Component> createBookmarkComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createCodeComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createCustomTrailComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createErrorComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createGraphComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createRefreshComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createScreenSearchComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createSearchComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createStatusBarComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createStatusComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createTabsComponent(ViewLayout* viewLayout, ScreenSearchSender* screenSearchSender);
  std::shared_ptr<Component> createTooltipComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createUndoRedoComponent(ViewLayout* viewLayout);
  std::shared_ptr<Component> createChatComponent(ViewLayout* viewLayout,
                                                 std::shared_ptr<ChatModel> model,
                                                 std::shared_ptr<sourcetrail::lib_llm::services::LlmCoordinator> coordinator);

private:
  const ViewFactory* m_viewFactory;
  StorageAccess* m_storageAccess;
};
