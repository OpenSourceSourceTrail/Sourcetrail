#pragma once
// internal
#include "Message.h"
#include "MessageActivateBase.h"
#include "GlobalId.hpp"
//
#include "SearchMatch.h"

class MessageActivateTokens
    : public Message<MessageActivateTokens>
    , public MessageActivateBase {
public:
  static const std::string getStaticType() {
    return "MessageActivateTokens";
  }

  MessageActivateTokens(const MessageBase* other) : isEdge(false), isBundledEdges(false), isFromSearch(false) {
    setIsParallel(true);
    setKeepContent(other->keepContent());
    setSchedulerId(other->getSchedulerId());
  }

  void print(std::wostream& ostream) const override {
    for(const Id& id : tokenIds) {
      ostream << id << L" ";
    }

    for(const SearchMatch& match : searchMatches) {
      for(const NameHierarchy& name : match.tokenNames) {
        ostream << name.getQualifiedName() << L" ";
      }
    }
  }

  std::vector<SearchMatch> getSearchMatches() const override {
    if(isBundledEdges) {
      SearchMatch match;
      match.name = match.text = L"bundled edges";    // TODO: show bundled edges source and target
      match.searchType = SearchMatch::SEARCH_TOKEN;
      match.nodeType = NodeType(NODE_TYPE);
      return {match};
    }

    return searchMatches;
  }

  std::vector<Id> tokenIds;
  std::vector<SearchMatch> searchMatches;

  bool isEdge;
  bool isBundledEdges;
  bool isFromSearch;
};