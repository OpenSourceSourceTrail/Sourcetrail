#pragma once
#include "Controller.h"
#include "MessageListener.h"
#include "Status.h"
#include "type/MessageClearStatusView.h"
#include "type/MessageShowStatus.h"
#include "type/MessageStatus.h"
#include "type/MessageStatusFilterChanged.h"

class StatusView;

class StatusController final
    : public Controller
    , public MessageListener<MessageClearStatusView>
    , public MessageListener<MessageShowStatus>
    , public MessageListener<MessageStatus>
    , public MessageListener<MessageStatusFilterChanged> {
public:
  StatusController();

  StatusController(const StatusController& viewLayout) = delete;
  StatusController(StatusController&& viewLayout) = delete;
  StatusController& operator=(const StatusController& viewLayout) = delete;
  StatusController& operator=(StatusController&& viewLayout) = delete;

  ~StatusController() override;

private:
  [[nodiscard]] StatusView* getView() const;

  void clear() override;

  void handleMessage(MessageClearStatusView* message) override;
  void handleMessage(MessageShowStatus* message) override;
  void handleMessage(MessageStatus* message) override;
  void handleMessage(MessageStatusFilterChanged* message) override;

  void addStatus(const std::vector<Status>& statuses);

  std::vector<Status> mStatus;
  StatusFilter mStatusFilter;
};