#pragma once
// internal
#include "FilePath.h"
#include "Message.h"
#include "TabId.h"

class MessageScrollToLine final : public Message<MessageScrollToLine> {
public:
  MessageScrollToLine(const FilePath& filePath_, size_t line_) : filePath(filePath_), line(line_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageScrollToLine";
  }

  void print(std::wostream& ostream) const override {
    ostream << filePath.wstr() << L":" << line;
  }

  const FilePath filePath;
  size_t line;
};
