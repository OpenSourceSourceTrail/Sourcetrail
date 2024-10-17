#pragma once
// STL
#include <memory>
// internal
#include "Controller.h"
// message
#include "MessageListener.h"
#include "type/code/MessageActivateFile.h"
#include "type/code/MessageActivateSourceLocations.h"
#include "type/code/MessageActivateTokenIds.h"
#include "type/graph/MessageActivateEdge.h"
#include "type/graph/MessageActivateNodes.h"
#include "type/MessageResetZoom.h"
#include "type/MessageZoom.h"
#include "type/search/MessageSearch.h"

class StorageAccess;

class ActivationController
    : public Controller
    , public MessageListener<MessageActivateEdge>
    , public MessageListener<MessageActivateFile>
    , public MessageListener<MessageActivateNodes>
    , public MessageListener<MessageActivateTokenIds>
    , public MessageListener<MessageActivateSourceLocations>
    , public MessageListener<MessageResetZoom>
    , public MessageListener<MessageSearch>
    , public MessageListener<MessageZoom> {
public:
  explicit ActivationController(StorageAccess* storageAccess);

  void clear() override;

private:
  void handleMessage(MessageActivateEdge* message) override;
  void handleMessage(MessageActivateFile* message) override;
  void handleMessage(MessageActivateNodes* message) override;
  void handleMessage(MessageActivateTokenIds* message) override;
  void handleMessage(MessageActivateSourceLocations* message) override;
  void handleMessage(MessageResetZoom* message) override;
  void handleMessage(MessageSearch* message) override;
  void handleMessage(MessageZoom* message) override;

  StorageAccess* m_storageAccess;
};
