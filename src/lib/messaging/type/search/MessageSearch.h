#pragma once
// internal
#include "Message.h"
//
#include "NodeTypeSet.h"
#include "SearchMatch.h"
#include "TabId.h"

class MessageSearch final : public Message<MessageSearch> {
public:
  static std::string getStaticType() {
    return "MessageSearch";
  }

  MessageSearch(const std::vector<SearchMatch>& matches, NodeTypeSet acceptedNodeTypes_)
      : acceptedNodeTypes(acceptedNodeTypes_), m_matches(matches) {
    setSchedulerId(TabId::currentTab());
  }

  [[nodiscard]] const std::vector<SearchMatch>& getMatches() const {
    return m_matches;
  }

  void print(std::wostream& ostream) const override {
    for(const SearchMatch& match : m_matches) {
      ostream << " @" << match.name;
      for(Id id : match.tokenIds) {
        ostream << ' ' << id;
      }
    }
  }

  NodeTypeSet acceptedNodeTypes;

private:
  const std::vector<SearchMatch> m_matches;
};