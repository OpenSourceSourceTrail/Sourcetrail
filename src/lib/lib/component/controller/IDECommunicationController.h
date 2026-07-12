#pragma once
#include <string>

#include "Controller.h"
#include "MessageListener.h"
#include "NetworkProtocolHelper.h"
#include "type/MessageWindowFocus.h"
#include "type/plugin/MessageIDECreateCDB.h"
#include "type/plugin/MessageMoveIDECursor.h"
#include "type/plugin/MessagePluginPortChange.h"

class StorageAccess;

class IDECommunicationController
    : public Controller
    , public MessageListener<MessageWindowFocus>
    , public MessageListener<MessageIDECreateCDB>
    , public MessageListener<MessageMoveIDECursor>
    , public MessageListener<MessagePluginPortChange> {
public:
  explicit IDECommunicationController(StorageAccess* storageAccess);
  ~IDECommunicationController() override;

  void clear() override;

  virtual void startListening() = 0;
  virtual void stopListening() = 0;
  virtual bool isListening() const = 0;

  void handleIncomingMessage(const std::wstring& message);

  bool getEnabled() const;
  void setEnabled(const bool enabled);

protected:
  void sendUpdatePing() const;

private:
  void handleSetActiveTokenMessage(const NetworkProtocolHelper::SetActiveTokenMessage& message) const;
  void handleCreateProjectMessage(const NetworkProtocolHelper::CreateProjectMessage& message);
  void handleCreateCDBProjectMessage(const NetworkProtocolHelper::CreateCDBProjectMessage& message);
  void handlePing(const NetworkProtocolHelper::PingMessage& message);

  void handleMessage(MessageWindowFocus* message) override;
  void handleMessage(MessageIDECreateCDB* message) override;
  void handleMessage(MessageMoveIDECursor* message) override;
  void handleMessage(MessagePluginPortChange* message) override;
  virtual void sendMessage(const std::wstring& message) const = 0;

  StorageAccess* m_storageAccess;

  bool m_enabled = true;
};
