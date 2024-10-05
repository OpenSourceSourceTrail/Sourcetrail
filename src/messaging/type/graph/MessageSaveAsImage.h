#pragma once
// Qt5
#include <QString>
// internal
#include "Message.h"

class MessageSaveAsImage final : public Message<MessageSaveAsImage> {
public:
  MessageSaveAsImage(QString path_) : path(std::move(path_)) {}

  static const std::string getStaticType() {
    return "MessageSaveAsImage";
  }

  QString path;
};