#pragma once
// internal
#include "Message.h"

class MessageIDECreateCDB final : public Message<MessageIDECreateCDB> {
public:
  MessageIDECreateCDB() = default;

  static const std::string getStaticType() {
    return "MessageIDECreateCDB";
  }

  void print(std::wostream& ostream) const override {
    ostream << L"Create CDB from current solution";
  }
};