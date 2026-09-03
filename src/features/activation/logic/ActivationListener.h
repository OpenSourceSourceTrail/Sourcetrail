#pragma once
// internal
#include "activation/messages/MessageActivateErrors.h"
#include "activation/messages/MessageActivateFullTextSearch.h"
#include "activation/messages/MessageActivateLegend.h"
#include "activation/messages/MessageActivateOverview.h"
#include "activation/messages/MessageActivateTokens.h"
#include "activation/messages/MessageActivateTrail.h"
#include "MessageListener.h"

class ActivationListener
    : public MessageListener<MessageActivateErrors>
    , public MessageListener<MessageActivateFullTextSearch>
    , public MessageListener<MessageActivateOverview>
    , public MessageListener<MessageActivateLegend>
    , public MessageListener<MessageActivateTokens>
    , public MessageListener<MessageActivateTrail> {
protected:
  const std::vector<SearchMatch>& getSearchMatches() const;

private:
  void handleMessage(MessageActivateErrors* message) override;
  void handleMessage(MessageActivateFullTextSearch* message) override;
  void handleMessage(MessageActivateOverview* message) override;
  void handleMessage(MessageActivateLegend* message) override;
  void handleMessage(MessageActivateTokens* message) override;
  void handleMessage(MessageActivateTrail* message) override;

  void handleMessageBase(const MessageActivateBase* message);

  virtual void handleActivation(const MessageActivateBase* message);
  virtual void handleActivation(const std::vector<SearchMatch>& searchMatches);

  std::vector<SearchMatch> m_searchMatches;
};
