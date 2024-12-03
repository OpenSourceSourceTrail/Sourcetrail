#pragma once
// internal
#include "GlobalId.hpp"
#include "Message.h"
#include "MessageActivateBase.h"
#include "TabId.h"

class MessageActivateTrail
    : public Message<MessageActivateTrail>
    , public MessageActivateBase {
public:
  MessageActivateTrail(Id originId_, Id targetId_, Edge::TypeMask edgeTypes_, size_t depth_, bool horizontalLayout_)
      : originId(originId_)
      , targetId(targetId_)
      , nodeTypes(0)
      , edgeTypes(edgeTypes_)
      , nodeNonIndexed(false)
      , depth(depth_)
      , horizontalLayout(horizontalLayout_)
      , custom(false) {
    setSchedulerId(TabId::currentTab());
  }

  MessageActivateTrail(Id originId_,
                       Id targetId_,
                       NodeKindMask nodeTypes_,
                       Edge::TypeMask edgeTypes_,
                       bool nodeNonIndexed_,
                       size_t depth_,
                       bool horizontalLayout_)
      : originId(originId_)
      , targetId(targetId_)
      , nodeTypes(nodeTypes_)
      , edgeTypes(edgeTypes_)
      , nodeNonIndexed(nodeNonIndexed_)
      , depth(depth_)
      , horizontalLayout(horizontalLayout_)
      , custom(true) {
    setSchedulerId(TabId::currentTab());
  }

  static const std::string getStaticType() {
    return "MessageActivateTrail";
  }

  std::vector<SearchMatch> getSearchMatches() const override {
    return searchMatches;
  }

  std::vector<SearchMatch> searchMatches;

  const Id originId;
  const Id targetId;
  const NodeKindMask nodeTypes;
  const Edge::TypeMask edgeTypes;
  const bool nodeNonIndexed;
  const size_t depth;
  const bool horizontalLayout;
  const bool custom;
};