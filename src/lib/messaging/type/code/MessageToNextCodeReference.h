#pragma once
// internal
#include "FilePath.h"
#include "Message.h"
#include "TabId.h"

class MessageToNextCodeReference final : public Message<MessageToNextCodeReference> {
public:
  MessageToNextCodeReference(const FilePath& filePath_, size_t lineNumber_, size_t columnNumber_, bool next_)
      : filePath(filePath_), lineNumber(lineNumber_), columnNumber(columnNumber_), next(next_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageToNextCodeReference";
  }

  void print(std::wostream& ostream) const override {
    ostream << filePath.wstr() << L' ' << lineNumber << L':' << columnNumber << L' ';

    if(next) {
      ostream << L"next";
    } else {
      ostream << L"previous";
    }
  }

  const FilePath filePath;
  const size_t lineNumber;
  const size_t columnNumber;
  const bool next;
};