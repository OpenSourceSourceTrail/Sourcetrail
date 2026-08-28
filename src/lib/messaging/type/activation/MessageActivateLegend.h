#pragma once
// internal
#include "component/TabId.h"
#include "Message.h"
#include "MessageActivateBase.h"

class MessageActivateLegend
    : public Message<MessageActivateLegend>
    , public MessageActivateBase {
public:
  MessageActivateLegend() {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageActivateLegend";
  }

  std::vector<SearchMatch> getSearchMatches() const override {
    return {SearchMatch::createCommand(SearchMatch::COMMAND_LEGEND)};
  }
};