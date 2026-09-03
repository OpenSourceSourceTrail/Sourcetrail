#ifndef SCREEN_SEARCH_CONTROLLER_H
#define SCREEN_SEARCH_CONTROLLER_H

#include <mutex>
#include <set>

#include "activation/logic/ActivationListener.h"
#include "code/messages/MessageActivateLocalSymbols.h"
#include "code/messages/MessageChangeFileView.h"
#include "code/messages/MessageCodeShowDefinition.h"
#include "component/controller/Controller.h"
#include "graph/messages/MessageActivateTrailEdge.h"
#include "graph/messages/MessageDeactivateEdge.h"
#include "graph/messages/MessageGraphNodeBundleSplit.h"
#include "graph/messages/MessageGraphNodeExpand.h"
#include "graph/messages/MessageGraphNodeHide.h"
#include "MessageListener.h"
#include "search/logic/ScreenSearchInterfaces.h"

class ScreenSearchController
    : public Controller
    , public ScreenSearchSender
    , public ActivationListener
    , public MessageListener<MessageActivateLocalSymbols>
    , public MessageListener<MessageActivateTrailEdge>
    , public MessageListener<MessageChangeFileView>
    , public MessageListener<MessageCodeShowDefinition>
    , public MessageListener<MessageDeactivateEdge>
    , public MessageListener<MessageGraphNodeBundleSplit>
    , public MessageListener<MessageGraphNodeExpand>
    , public MessageListener<MessageGraphNodeHide> {
public:
  virtual ~ScreenSearchController() = default;

  // Controller implementation
  void clear() override;

  // ScreenSearchSender implementation
  void foundMatches(ScreenSearchResponder* responder, size_t matchCount) override;
  void addResponder(ScreenSearchResponder* responder) override;
  void removeResponder(ScreenSearchResponder* responder) override;
  void clearMatches() override;

  void search(const std::wstring& query, const std::set<std::string>& responderNames);
  void activateMatch(bool next);

private:
  size_t getResponderId(ScreenSearchResponder* responder) const;

  void handleActivation(const MessageActivateBase* message) override;

  void handleMessage(MessageActivateLocalSymbols* message) override;
  void handleMessage(MessageActivateTrailEdge* message) override;
  void handleMessage(MessageChangeFileView* message) override;
  void handleMessage(MessageCodeShowDefinition* message) override;
  void handleMessage(MessageDeactivateEdge* message) override;
  void handleMessage(MessageGraphNodeBundleSplit* message) override;
  void handleMessage(MessageGraphNodeExpand* message) override;
  void handleMessage(MessageGraphNodeHide* message) override;

  std::vector<ScreenSearchResponder*> m_responders;

  std::vector<std::pair<size_t, size_t>> m_matches;
  size_t m_matchIndex = 0;
  std::mutex m_matchMutex;
};

#endif    // SCREEN_SEARCH_CONTROLLER_H
