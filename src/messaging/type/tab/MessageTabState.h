#pragma once
// internal
#include "Message.h"
#include "SearchMatch.h"

class MessageTabState final : public Message<MessageTabState> {
public:
  MessageTabState(Id tabId_, std::vector<SearchMatch> searchMatches_) : tabId(tabId_), searchMatches(std::move(searchMatches_)) {}

  static const std::string getStaticType() {
    return "MessageTabState";
  }

  const Id tabId;
  const std::vector<SearchMatch> searchMatches;
};