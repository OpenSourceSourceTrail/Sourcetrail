#pragma once
// internal
#include "Message.h"
#include "TabId.h"

class MessageCodeReference final : public Message<MessageCodeReference> {
public:
  enum ReferenceType { REFERENCE_PREVIOUS, REFERENCE_NEXT };

  MessageCodeReference(ReferenceType type_, bool localReference_) : type(type_), localReference(localReference_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageCodeReference";
  }

  void print(std::wostream& ostream) const override {
    if(type == REFERENCE_PREVIOUS) {
      ostream << L"previous";
    } else {
      ostream << L"next";
    }

    if(localReference) {
      ostream << L" local";
    }
  }

  const ReferenceType type;
  bool localReference;
};