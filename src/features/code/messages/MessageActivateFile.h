#pragma once
// STL
#include <cstdint>
// internal
#include "FilePath.h"
#include "Message.h"
#include "TabId.h"

class MessageActivateFile final : public Message<MessageActivateFile> {
public:
  MessageActivateFile(const FilePath& filePath_, unsigned int line_ = 0) : filePath(filePath_), line(line_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageActivateFile";
  }

  void print(std::wostream& ostream) const override {
    ostream << filePath.wstr();
  }

  const FilePath filePath;
  uint32_t line;
};