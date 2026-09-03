#pragma once
#include <string>

#include "component/controller/Controller.h"
#include "error/messages/MessageErrorCountClear.h"
#include "error/messages/MessageErrorCountUpdate.h"
#include "ide_communication/messages/MessagePingReceived.h"
#include "indexing/messages/MessageIndexingFinished.h"
#include "indexing/messages/MessageIndexingStarted.h"
#include "indexing/messages/MessageIndexingStatus.h"
#include "MessageListener.h"
#include "refresh/messages/MessageRefresh.h"
#include "status/messages/MessageStatus.h"

class StatusBarView;
class StorageAccess;

class StatusBarController final
    : public Controller
    , public MessageListener<MessageErrorCountClear>
    , public MessageListener<MessageErrorCountUpdate>
    , public MessageListener<MessageIndexingFinished>
    , public MessageListener<MessageIndexingStarted>
    , public MessageListener<MessageIndexingStatus>
    , public MessageListener<MessagePingReceived>
    , public MessageListener<MessageRefresh>
    , public MessageListener<MessageStatus> {
public:
  explicit StatusBarController(StorageAccess* storageAccess);

  StatusBarController(const StatusBarController&) = delete;
  StatusBarController(StatusBarController&&) = delete;
  StatusBarController& operator=(const StatusBarController&) = delete;
  StatusBarController& operator=(StatusBarController&&) = delete;

  ~StatusBarController() override;

  [[maybe_unused]] StatusBarView* getView();

  void clear() override;

private:
  void handleMessage(MessageErrorCountClear* message) override;
  void handleMessage(MessageErrorCountUpdate* message) override;
  void handleMessage(MessageIndexingFinished* message) override;
  void handleMessage(MessageIndexingStarted* message) override;
  void handleMessage(MessageIndexingStatus* message) override;
  void handleMessage(MessagePingReceived* message) override;
  void handleMessage(MessageRefresh* message) override;
  void handleMessage(MessageStatus* message) override;

  void setStatus(const std::wstring& status, bool isError, bool showLoader);

  const StorageAccess* mStorageAccess;
};
