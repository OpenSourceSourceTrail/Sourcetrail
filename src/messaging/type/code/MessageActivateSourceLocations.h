#pragma once
// internal
#include "Message.h"
#include "TabId.h"
#include "GlobalId.hpp"

class MessageActivateSourceLocations final : public Message<MessageActivateSourceLocations> {
public:
  MessageActivateSourceLocations(const std::vector<Id>& locationIds_, bool containsUnsolvedLocations_)
      : locationIds(locationIds_), containsUnsolvedLocations(containsUnsolvedLocations_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageActivateSourceLocations";
  }

  void print(std::wostream& ostream) const override {
    for(const Id& id : locationIds) {
      ostream << id << L" ";
    }
  }

  const std::vector<Id> locationIds;
  const bool containsUnsolvedLocations;
};