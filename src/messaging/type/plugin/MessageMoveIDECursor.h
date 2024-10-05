#pragma once
// internal
#include "FilePath.h"
#include "Message.h"

class MessageMoveIDECursor final : public Message<MessageMoveIDECursor> {
public:
  MessageMoveIDECursor(const FilePath& filePath_, unsigned int row_, unsigned int column_)
      : filePath(filePath_), row(row_), column(column_) {}

  static const std::string getStaticType() {
    return "MessageMoveIDECursor";
  }

  virtual void print(std::wostream& ostream) const {
    ostream << filePath.wstr() << L":" << row << L":" << column;
  }

  const FilePath filePath;
  const unsigned int row;
  const unsigned int column;
};
