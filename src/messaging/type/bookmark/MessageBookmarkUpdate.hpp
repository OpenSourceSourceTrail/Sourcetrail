#pragma once
#include "Bookmark.h"
#include "Message.h"

class MessageBookmarkUpdate : public Message<MessageBookmarkUpdate> {
public:
  MessageBookmarkUpdate(std::vector<std::shared_ptr<Bookmark>> bookmarks) : mBookmarks(std::move(bookmarks)) {}

  static const std::string getStaticType() {
    return "MessageBookmarkUpdate";
  }

  void print(std::wostream& os) const override {
    os << utility::decodeFromUtf8(getStaticType());
  }

  std::vector<std::shared_ptr<Bookmark>> mBookmarks;
};