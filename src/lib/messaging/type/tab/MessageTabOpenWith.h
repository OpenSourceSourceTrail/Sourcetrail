#pragma once
// internal
#include "FilePath.h"
#include "Message.h"
#include "SearchMatch.h"

class MessageTabOpenWith final : public Message<MessageTabOpenWith> {
public:
  static const std::string getStaticType() {
    return "MessageTabOpenWith";
  }

  MessageTabOpenWith(Id tokenId_, Id locationId_ = 0) : tokenId(tokenId_), locationId(locationId_) {}

  MessageTabOpenWith(const FilePath& path, size_t line_ = 0) : filePath(path), line(line_) {}

  MessageTabOpenWith(const SearchMatch& match_) : match(match_) {}

  MessageTabOpenWith& showNewTab(bool show) {
    showTab = show;
    return *this;
  }

  const Id tokenId = 0;
  const Id locationId = 0;

  const FilePath filePath;
  const SearchMatch match;

  const size_t line = 0;

  bool showTab = false;
};