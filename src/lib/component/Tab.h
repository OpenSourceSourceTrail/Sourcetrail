#pragma once
// STL
#include <vector>
// internal
#include "ComponentManager.h"
#include "type/focus/MessageFocusView.h"
#include "MessageListener.h"
#include "type/MessageRefreshUI.h"
#include "View.h"
#include "ViewLayout.h"

class Tab final
    : public ViewLayout
    , public MessageListener<MessageFocusView>
    , public MessageListener<MessageRefreshUI> {
public:
  Tab(Id tabId, const ViewFactory* viewFactory, StorageAccess* storageAccess, ScreenSearchSender* screenSearchSender);
  ~Tab() override;

  [[nodiscard]] Id getSchedulerId() const override;

  void setParentLayout(ViewLayout* parentLayout);

  // ViewLayout implementation
  void addView(View* view) override;
  void removeView(View* view) override;

  void showView(View* view) override;
  void hideView(View* view) override;

  void setViewEnabled(View* view, bool enabled) override;

private:
  void handleMessage(MessageFocusView* message) override;
  void handleMessage(MessageRefreshUI* message) override;

  const Id m_tabId;

  ComponentManager m_componentManager;
  std::vector<View*> m_views;

  ViewLayout* m_parentLayout;
  ScreenSearchSender* m_screenSearchSender;
};