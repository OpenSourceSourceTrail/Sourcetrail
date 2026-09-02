#ifndef TABS_CONTROLLER_H
#define TABS_CONTROLLER_H

#include <functional>

#include "activation/messages/MessageActivateErrors.h"
#include "component/controller/Controller.h"
#include "component/Tab.h"
#include "indexing/messages/MessageIndexingFinished.h"
#include "MessageListener.h"
#include "tabs/logic/TabsView.h"
#include "tabs/messages/MessageTabClose.h"
#include "tabs/messages/MessageTabOpen.h"
#include "tabs/messages/MessageTabOpenWith.h"
#include "tabs/messages/MessageTabSelect.h"
#include "tabs/messages/MessageTabState.h"

struct SearchMatch;

class StorageAccess;
class ViewFactory;
class ViewLayout;

class TabsController
    : public Controller
    , public MessageListener<MessageActivateErrors>
    , public MessageListener<MessageIndexingFinished>
    , public MessageListener<MessageTabClose>
    , public MessageListener<MessageTabOpen>
    , public MessageListener<MessageTabOpenWith>
    , public MessageListener<MessageTabSelect>
    , public MessageListener<MessageTabState> {
public:
  /**
   * `isProjectLoaded` answers whether a project is open; a tab is only ever opened when one is.
   *
   * Injected rather than read off Application::getInstance(), which is the only thing this
   * controller ever wanted from it -- and asking for it made the controller unconstructible
   * without a whole Application behind it.
   */
  TabsController(ViewLayout* mainLayout,
                 const ViewFactory* viewFactory,
                 StorageAccess* storageAccess,
                 ScreenSearchSender* screenSearchSender,
                 std::function<bool()> isProjectLoaded);

  // Controller implementation
  virtual void clear();

  void addTab(Id tabId, SearchMatch match);
  void showTab(Id tabId);
  void removeTab(Id tabId);
  void destroyTab(Id tabId);
  void onClearTabs();

private:
  virtual void handleMessage(MessageActivateErrors* message);
  virtual void handleMessage(MessageIndexingFinished* message);
  virtual void handleMessage(MessageTabClose* message);
  virtual void handleMessage(MessageTabOpen* message);
  virtual void handleMessage(MessageTabOpenWith* message);
  virtual void handleMessage(MessageTabSelect* message);
  virtual void handleMessage(MessageTabState* message);

  TabsView* getView() const;

  ViewLayout* m_mainLayout;
  const ViewFactory* m_viewFactory;
  StorageAccess* m_storageAccess;
  ScreenSearchSender* m_screenSearchSender;
  std::function<bool()> m_isProjectLoaded;

  std::map<Id, std::shared_ptr<Tab>> m_tabs;
  std::mutex m_tabsMutex;

  bool m_isCreatingTab;
  std::tuple<Id, FilePath, size_t> m_scrollToLine;
};

#endif    // TABS_CONTROLLER_H
