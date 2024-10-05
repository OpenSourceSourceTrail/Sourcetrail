#pragma once
// internal
#include "Message.h"

class MessageZoom : public Message<MessageZoom> {
public:
  MessageZoom(bool zoomIn_) : zoomIn(zoomIn_) {}

  static const std::string getStaticType() {
    return "MessageZoom";
  }

  void print(std::wostream& ostream) const override {
    if(zoomIn) {
      ostream << L"in";
    } else {
      ostream << L"out";
    }
  }

  const bool zoomIn;
};
