#include "IDECommunicationController.h"

#include "FileSystem.h"
#include "logging.h"
#include "SourceLocationFile.h"
#include "StorageAccess.h"
#include "type/MessageActivateWindow.h"
#include "type/MessageProjectNew.h"
#include "type/MessageStatus.h"
#include "type/plugin/MessagePingReceived.h"
#include "type/tab/MessageTabOpenWith.h"

IDECommunicationController::IDECommunicationController(StorageAccess* storageAccess) : m_storageAccess(storageAccess) {}

IDECommunicationController::~IDECommunicationController() = default;

void IDECommunicationController::clear() {}

void IDECommunicationController::handleIncomingMessage(const std::wstring& message) {
  if(!m_enabled) {
    return;
  }

  const NetworkProtocolHelper::MESSAGE_TYPE type = NetworkProtocolHelper::getMessageType(message);
  if(type == NetworkProtocolHelper::MESSAGE_TYPE::UNKNOWN) {
    LOG_ERROR("Invalid message type");
  } else if(type == NetworkProtocolHelper::MESSAGE_TYPE::SET_ACTIVE_TOKEN) {
    handleSetActiveTokenMessage(NetworkProtocolHelper::parseSetActiveTokenMessage(message));
  } else if(type == NetworkProtocolHelper::MESSAGE_TYPE::CREATE_CDB_MESSAGE) {
    handleCreateCDBProjectMessage(NetworkProtocolHelper::parseCreateCDBProjectMessage(message));
  } else if(type == NetworkProtocolHelper::MESSAGE_TYPE::PING) {
    handlePing(NetworkProtocolHelper::parsePingMessage(message));
  } else {
    handleCreateProjectMessage(NetworkProtocolHelper::parseCreateProjectMessage(message));
  }
}

bool IDECommunicationController::getEnabled() const {
  return m_enabled;
}

void IDECommunicationController::setEnabled(const bool enabled) {
  m_enabled = enabled;
}

void IDECommunicationController::sendUpdatePing() const {
  // first reset connection status
  MessagePingReceived{}.dispatch();

  // send ping to update connection status
  sendMessage(NetworkProtocolHelper::buildPingMessage());
}

void IDECommunicationController::handleSetActiveTokenMessage(const NetworkProtocolHelper::SetActiveTokenMessage& message) const {
  if(message.valid) {
    const unsigned int cursorColumn = message.column;

    const Id fileId = m_storageAccess->getNodeIdForFileNode(message.filePath.getCanonical());
    const FileInfo fileInfo = m_storageAccess->getFileInfoForFileId(fileId);
    const FilePath filePath = fileInfo.path;

    if(FileSystem::getFileInfoForPath(filePath).lastWriteTime == fileInfo.lastWriteTime) {
      // file was not modified
      const std::shared_ptr<SourceLocationFile> sourceLocationFile = m_storageAccess->getSourceLocationsForLinesInFile(
          filePath, message.row, message.row);

      std::vector<Id> selectedLocationIds;
      sourceLocationFile->forEachStartSourceLocation([&selectedLocationIds, &cursorColumn](SourceLocation* startLocation) {
        const SourceLocation* endLocation = startLocation->getEndLocation();

        if((startLocation->getType() == LOCATION_TOKEN || startLocation->getType() == LOCATION_QUALIFIER ||
            startLocation->getType() == LOCATION_UNSOLVED) &&
           startLocation->getLineNumber() == endLocation->getLineNumber() && startLocation->getColumnNumber() <= cursorColumn &&
           endLocation->getColumnNumber() + 1 >= cursorColumn) {
          selectedLocationIds.push_back(startLocation->getLocationId());
        }
      });

      if(!selectedLocationIds.empty()) {
        MessageStatus(L"Activating source location from plug-in succeeded: " + filePath.wstr() + L", row: " +
                      std::to_wstring(message.row) + L", col: " + std::to_wstring(message.column))
            .dispatch();

        MessageTabOpenWith(0, selectedLocationIds[0]).showNewTab(true).dispatch();
        MessageActivateWindow().dispatch();
        return;
      }
    }

    if(fileId > 0) {
      MessageTabOpenWith(filePath, message.row).showNewTab(true).dispatch();
      MessageActivateWindow().dispatch();
    } else {
      MessageStatus{
          L"Activating source location from plug-in failed. File " + filePath.wstr() + L" was not found in the project.", true}
          .dispatch();
    }
  }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void IDECommunicationController::handleCreateProjectMessage(const NetworkProtocolHelper::CreateProjectMessage& /*message*/) {
  LOG_ERROR("Network Protocol CreateProjectMessage not supported anymore.");
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void IDECommunicationController::handleCreateCDBProjectMessage(const NetworkProtocolHelper::CreateCDBProjectMessage& message) {
  if(message.valid) {
    MessageProjectNew{message.cdbFileLocation}.dispatch();
  } else {
    LOG_ERROR("Unable to parse provided CDB, invalid data received");
  }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void IDECommunicationController::handlePing(const NetworkProtocolHelper::PingMessage& message) {
  if(message.valid) {
    MessagePingReceived msg;
    msg.ideName = message.ideId;

    if(msg.ideName.empty()) {
      msg.ideName = L"unknown IDE";
    }

    LOG_INFO(fmt::format(L"{} instance detected via plugin port", msg.ideName));
    msg.dispatch();
  } else {
    LOG_ERROR("Can't handle ping, message is invalid");
  }
}

void IDECommunicationController::handleMessage(MessageWindowFocus* message) {
  if(message->focusIn) {
    sendUpdatePing();
  }
}

void IDECommunicationController::handleMessage(MessageIDECreateCDB* /*message*/) {
  const std::wstring networkMessage = NetworkProtocolHelper::buildCreateCDBMessage();

  MessageStatus(L"Requesting IDE to create Compilation Database via plug-in.").dispatch();

  sendMessage(networkMessage);
}

void IDECommunicationController::handleMessage(MessageMoveIDECursor* message) {
  const std::wstring networkMessage = NetworkProtocolHelper::buildSetIDECursorMessage(
      message->filePath, message->row, message->column);

  MessageStatus(L"Jump to source location via plug-in: " + message->filePath.wstr() + L", row: " + std::to_wstring(message->row) +
                L", col: " + std::to_wstring(message->column))
      .dispatch();

  sendMessage(networkMessage);
}

void IDECommunicationController::handleMessage(MessagePluginPortChange* /*message*/) {
  stopListening();
  startListening();
}
