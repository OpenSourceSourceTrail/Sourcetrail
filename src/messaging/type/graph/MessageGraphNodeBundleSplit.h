#pragma once
// internal
#include "Message.h"
#include "TabId.h"
#include "GlobalId.hpp"

class MessageGraphNodeBundleSplit final : public Message<MessageGraphNodeBundleSplit> {
public:
  MessageGraphNodeBundleSplit(Id bundleId_, bool removeOtherNodes_ = false, bool layoutToList_ = false)
      : bundleId(bundleId_), removeOtherNodes(removeOtherNodes_), layoutToList(layoutToList_) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageGraphNodeBundleSplit";
  }

  virtual void print(std::wostream& ostream) const {
    ostream << bundleId;
  }

  Id bundleId;
  bool removeOtherNodes;
  bool layoutToList;
};