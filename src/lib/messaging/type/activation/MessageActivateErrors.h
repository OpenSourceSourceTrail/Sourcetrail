#pragma once
// internal
#include "Message.h"
#include "MessageActivateBase.h"
//
#include "ErrorFilter.h"
#include "TabId.h"

class MessageActivateErrors
    : public Message<MessageActivateErrors>
    , public MessageActivateBase {
public:
  static const std::string getStaticType() {
    return "MessageActivateErrors";
  }

  MessageActivateErrors(const ErrorFilter& filter_, const FilePath& file_ = FilePath()) : filter(filter_), file(file_) {
    setSchedulerId(TabId::currentTab());
  }

  std::vector<SearchMatch> getSearchMatches() const override {
    std::vector<SearchMatch> matches = {SearchMatch::createCommand(SearchMatch::COMMAND_ERROR)};
    if(!file.empty()) {
      SearchMatch match;
      match.name = match.text = file.fileName();
      match.searchType = SearchMatch::SEARCH_TOKEN;
      match.nodeType = NodeType(NODE_FILE);
      matches.push_back(match);
    }
    return matches;
  }

  const ErrorFilter filter;
  const FilePath file;
};