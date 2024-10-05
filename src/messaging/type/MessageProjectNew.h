#pragma once
// internal
#include "FilePath.h"
#include "Message.h"

class MessageProjectNew : public Message<MessageProjectNew> {
public:
  MessageProjectNew(const FilePath& cdbPath_) : cdbPath(cdbPath_) {}

  static const std::string getStaticType() {
    return "MessageProjectNew";
  }

  const FilePath cdbPath;
};
