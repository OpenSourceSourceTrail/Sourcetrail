#pragma once
// internal
#include "Message.h"
#include "TabId.h"
#include "types.h"

class MessageActivateLocalSymbols final : public Message<MessageActivateLocalSymbols> {
public:
  MessageActivateLocalSymbols(const std::vector<Id>& symbolIds_) : symbolIds(symbolIds_) {
    setSchedulerId(TabId::currentTab());
  }

  void addSymbol(Id symbolId) {
    symbolIds.push_back(symbolId);
  }

  static const std::string getStaticType() {
    return "MessageActivateLocalSymbols";
  }

  void print(std::wostream& ostream) const override {
    for(const Id& symbolId : symbolIds) {
      ostream << symbolId << L" ";
    }
  }

  std::vector<Id> symbolIds;
};