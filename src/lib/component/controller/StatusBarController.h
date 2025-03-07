#pragma once
#include <string>

#include "Controller.h"
#include "MessageListener.h"
#include "type/error/MessageErrorCountClear.h"
#include "type/error/MessageErrorCountUpdate.h"
#include "type/indexing/MessageIndexingFinished.h"
#include "type/indexing/MessageIndexingStarted.h"
#include "type/indexing/MessageIndexingStatus.h"
#include "type/MessageRefresh.h"
#include "type/MessageStatus.h"
#include "type/plugin/MessagePingReceived.h"

class StatusBarView;
class StorageAccess;

class StatusBarController
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

  StorageAccess* mStorageAccess;
};
