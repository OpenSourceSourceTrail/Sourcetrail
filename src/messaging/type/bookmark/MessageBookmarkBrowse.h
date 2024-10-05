#pragma once
#include "Bookmark.h"
#include "Message.h"

class MessageBookmarkBrowse : public Message<MessageBookmarkBrowse> {
public:
  MessageBookmarkBrowse(Bookmark::Filter filter_ = Bookmark::Filter::Unknown,
                        Bookmark::Order order_ = Bookmark::Order::None)
      : filter(filter_), order(order_) {}

  static const std::string getStaticType() {
    return "MessageBookmarkBrowse";
  }

  const Bookmark::Filter filter;
  const Bookmark::Order order;
};
