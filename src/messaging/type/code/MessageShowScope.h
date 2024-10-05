#pragma once
// internal
#include "Message.h"
#include "TabId.h"
#include "types.h"

class MessageShowScope final : public Message<MessageShowScope> {
public:
  MessageShowScope(Id scopeLocationId_, bool showErrors_) : scopeLocationId(scopeLocationId_), showErrors(showErrors_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageShowScope";
  }

  void print(std::wostream& ostream) const override {
    ostream << scopeLocationId;
  }

  const Id scopeLocationId;
  const bool showErrors;
};