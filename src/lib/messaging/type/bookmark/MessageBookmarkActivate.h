#pragma once
// internal
#include "Bookmark.h"
#include "Message.h"

class MessageBookmarkActivate : public Message<MessageBookmarkActivate> {
public:
  MessageBookmarkActivate(std::shared_ptr<Bookmark> bookmark_) : bookmark(std::move(bookmark_)) {}

  static const std::string getStaticType() {
    return "MessageBookmarkActivate";
  }

  const std::shared_ptr<Bookmark> bookmark;
};
