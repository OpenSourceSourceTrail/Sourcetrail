#pragma once
// STL
#include <string>
// internal
#include "Message.h"

class MessageSaveAsImage final : public Message<MessageSaveAsImage> {
public:
  MessageSaveAsImage(std::wstring path_) : path(std::move(path_)) {}

  static const std::string getStaticType() {
    return "MessageSaveAsImage";
  }

  std::wstring path;
};
