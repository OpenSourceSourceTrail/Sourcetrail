#pragma once
// internal
#include "Message.h"
//
#include "TooltipInfo.h"
#include "TooltipOrigin.h"

class MessageTooltipShow final : public Message<MessageTooltipShow> {
public:
  MessageTooltipShow(TooltipInfo info_, TooltipOrigin origin_) : tooltipInfo(info_), origin(origin_) {
    setSendAsTask(false);
    setIsLogged(false);
  }

  MessageTooltipShow(const std::vector<Id>& sourceLocationIds_, const std::vector<Id>& localSymbolIds_, TooltipOrigin origin_)
      : sourceLocationIds(sourceLocationIds_), localSymbolIds(localSymbolIds_), origin(origin_) {
    setSendAsTask(false);
    setIsLogged(false);
  }

  static const std::string getStaticType() {
    return "MessageTooltipShow";
  }

  const TooltipInfo tooltipInfo;

  const std::vector<Id> sourceLocationIds;
  const std::vector<Id> localSymbolIds;

  const TooltipOrigin origin;

  bool force = false;
};