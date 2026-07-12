#pragma once
// internal
#include "Message.h"

class MessageCloseProject : public Message<MessageCloseProject> {
public:
  MessageCloseProject() {}

  static const std::string getStaticType() {
    return "MessageCloseProject";
  }

  void print(std::wostream& /*ostream*/) const override {}
};
