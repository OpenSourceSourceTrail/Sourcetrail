#pragma once
// STL
#include "FilePath.h"
#include "Message.h"

class MessageErrorsForFile final : public Message<MessageErrorsForFile> {
public:
  static const std::string getStaticType() {
    return "MessageErrorsForFile";
  }

  MessageErrorsForFile(const FilePath& file_) : file(file_) {}

  const FilePath& file;
};
