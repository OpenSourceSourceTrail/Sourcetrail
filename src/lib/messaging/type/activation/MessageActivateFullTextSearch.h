#pragma once
// internal
#include "Message.h"
#include "MessageActivateBase.h"
#include "TabId.h"

class MessageActivateFullTextSearch
    : public Message<MessageActivateFullTextSearch>
    , public MessageActivateBase {
public:
  static const std::string getStaticType() {
    return "MessageActivateFullTextSearch";
  }

  MessageActivateFullTextSearch(const std::wstring& searchTerm_, bool caseSensitive_ = false)
      : searchTerm(searchTerm_), caseSensitive(caseSensitive_) {
    setSchedulerId(TabId::currentTab());
  }

  void print(std::wostream& ostream) const override {
    ostream << searchTerm;
  }

  std::vector<SearchMatch> getSearchMatches() const override {
    std::wstring prefix(caseSensitive ? 2 : 1, SearchMatch::FULLTEXT_SEARCH_CHARACTER);
    SearchMatch match(prefix + searchTerm);
    match.searchType = SearchMatch::SEARCH_FULLTEXT;
    return {match};
  }

  const std::wstring searchTerm;
  bool caseSensitive;
};