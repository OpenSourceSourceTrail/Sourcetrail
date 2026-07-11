#pragma once
// internal
#include "MessageListener.h"
#include "type/activation/MessageActivateErrors.h"
#include "type/activation/MessageActivateFullTextSearch.h"
#include "type/activation/MessageActivateLegend.h"
#include "type/activation/MessageActivateOverview.h"
#include "type/activation/MessageActivateTokens.h"
#include "type/activation/MessageActivateTrail.h"

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
