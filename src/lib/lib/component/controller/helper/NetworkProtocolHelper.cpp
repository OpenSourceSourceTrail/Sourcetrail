#include "NetworkProtocolHelper.h"

#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

#include "logging.h"
#include "utilityString.h"

namespace {
constexpr size_t MaxMessageCount = 5;

constexpr std::wstring_view Divider = L">>";
constexpr std::wstring_view SetActiveTokenPrefix = L"setActiveToken";
constexpr std::wstring_view MoveCursorPrefix = L"moveCursor";
constexpr std::wstring_view EndOfMessageToken = L"<EOM>";
constexpr std::wstring_view CreateProjectPrefix = L"createProject";
constexpr std::wstring_view CreateCDBProjectPrefix = L"createCDBProject";
constexpr std::wstring_view CreateCDBPrefix = L"createCDB";
constexpr std::wstring_view PingPrefix = L"ping";

std::vector<std::wstring> divideMessage(const std::wstring& message) {
  std::vector<std::wstring> result;

  std::wstring msg = message;
  size_t pos = msg.find(Divider);

  while(pos != std::wstring::npos) {
    result.push_back(msg.substr(0, pos));
    msg = msg.substr(pos + Divider.size());
    pos = msg.find(Divider);
  }

  if(!msg.empty()) {
    pos = msg.find(EndOfMessageToken);
    if(pos != std::wstring::npos) {
      result.push_back(msg.substr(0, pos));
      result.push_back(msg.substr(pos));
    }
  }

  return result;
}

bool isDigits(const std::wstring& text) {
  return (text.find_first_not_of(L"0123456789") == std::wstring::npos);
}
}    // namespace

NetworkProtocolHelper::MESSAGE_TYPE NetworkProtocolHelper::getMessageType(const std::wstring& message) {
  std::vector<std::wstring> subMessages = divideMessage(message);

  if(!subMessages.empty()) {
    if(subMessages[0] == SetActiveTokenPrefix) {
      return SET_ACTIVE_TOKEN;
    }
    if(subMessages[0] == CreateProjectPrefix) {
      return CREATE_PROJECT;
    }
    if(subMessages[0] == CreateCDBProjectPrefix) {
      return CREATE_CDB_MESSAGE;
    }
    if(subMessages[0] == PingPrefix) {
      return PING;
    }
  }

  return UNKNOWN;
}

NetworkProtocolHelper::SetActiveTokenMessage NetworkProtocolHelper::parseSetActiveTokenMessage(const std::wstring& message) {
  const std::vector<std::wstring> subMessages = divideMessage(message);

  SetActiveTokenMessage networkMessage;

  if(!subMessages.empty()) {
    if(subMessages.front() == SetActiveTokenPrefix) {
      if(subMessages.size() != MaxMessageCount) {
        LOG_ERROR("Failed to parse setActiveToken message, invalid token count");
      } else {
        const std::wstring& filePath = subMessages[1];
        const std::wstring& row = subMessages[2];
        const std::wstring& column = subMessages[3];

        if(!filePath.empty() && !row.empty() && !column.empty() && isDigits(row) && isDigits(column)) {
          networkMessage.filePath = FilePath(filePath);
          networkMessage.row = static_cast<uint32_t>(std::stoi(row));
          networkMessage.column = static_cast<uint32_t>(std::stoi(column));
          networkMessage.valid = true;
        }
      }
    } else {
      LOG_ERROR(L"Failed to parse message, invalid type token: {}. Expected {}", subMessages.front(), SetActiveTokenPrefix);
    }
  }

  return networkMessage;
}

NetworkProtocolHelper::CreateProjectMessage NetworkProtocolHelper::parseCreateProjectMessage(const std::wstring& message) {
  const std::vector<std::wstring> subMessages = divideMessage(message);

  CreateProjectMessage networkMessage;

  if(!subMessages.empty()) {
    if(subMessages.front() == CreateProjectPrefix) {
      if(subMessages.size() != 4) {
        LOG_ERROR("Failed to parse createProject message, invalid token count");
      }
    } else {
      LOG_ERROR(L"Failed to parse message, invalid type token: {}. Expected {}", subMessages.front(), CreateProjectPrefix);
    }
  }

  return networkMessage;
}

NetworkProtocolHelper::CreateCDBProjectMessage NetworkProtocolHelper::parseCreateCDBProjectMessage(const std::wstring& message) {
  const std::vector<std::wstring> subMessages = divideMessage(message);

  CreateCDBProjectMessage networkMessage;

  if(!subMessages.empty()) {
    if(subMessages.front() == CreateCDBProjectPrefix) {
      if(subMessages.size() < 4) {
        LOG_ERROR("Failed to parse createCDBProject message, too few tokens");
      } else {
        const size_t subMessageCount = subMessages.size();

        const std::wstring& cdbPath = subMessages[1];
        if(!cdbPath.empty()) {
          networkMessage.cdbFileLocation = FilePath(cdbPath);
        } else {
          LOG_WARNING("CDB file path is not set.");
        }

        const std::wstring& ideId = subMessages[subMessageCount - 2];
        if(!ideId.empty()) {
          std::wstring nonConstId = ideId;
          boost::algorithm::to_lower(nonConstId);

          networkMessage.ideId = nonConstId;
        } else {
          LOG_WARNING(L"Failed to parse ide ID string. Is {}", ideId);
        }

        if(!networkMessage.cdbFileLocation.empty() && !networkMessage.ideId.empty()) {
          networkMessage.valid = true;
        }
      }
    } else {
      LOG_ERROR(L"Failed to parse message, invalid type token: {}. Expected {}", subMessages.front(), CreateCDBProjectPrefix);
    }
  }

  return networkMessage;
}

NetworkProtocolHelper::PingMessage NetworkProtocolHelper::parsePingMessage(const std::wstring& message) {
  const std::vector<std::wstring> subMessages = divideMessage(message);

  PingMessage pingMessage;

  if(!subMessages.empty()) {
    if(subMessages.front() == PingPrefix) {
      if(subMessages.size() < 2) {
        LOG_ERROR("Failed to parse PingMessage message, too few tokens");
      } else {
        const std::wstring& ideId = subMessages[1];

        if(!ideId.empty()) {
          std::wstring nonConstId = ideId;
          boost::algorithm::to_lower(nonConstId);
          pingMessage.ideId = ideId;
          pingMessage.valid = true;
        } else {
          LOG_WARNING(L"Failed to parse ide ID string: {}", ideId);
        }
      }
    } else {
      LOG_ERROR(L"Failed to parse message, invalid type token: {}. Expected {}", subMessages.front(), PingPrefix);
    }
  }

  return pingMessage;
}

std::wstring NetworkProtocolHelper::buildSetIDECursorMessage(const FilePath& fileLocation,
                                                             const unsigned int row,
                                                             const unsigned int column) {
  std::wstringstream messageStream;

  messageStream << MoveCursorPrefix;
  messageStream << Divider;
  messageStream << fileLocation.wstr();
  messageStream << Divider;
  messageStream << row;
  messageStream << Divider;
  messageStream << column;
  messageStream << EndOfMessageToken;

  return messageStream.str();
}

std::wstring NetworkProtocolHelper::buildCreateCDBMessage() {
  std::wstringstream messageStream;

  messageStream << CreateCDBPrefix;
  messageStream << EndOfMessageToken;

  return messageStream.str();
}

std::wstring NetworkProtocolHelper::buildPingMessage() {
  std::wstringstream messageStream;

  messageStream << PingPrefix;
  messageStream << Divider;
  messageStream << "sourcetrail";
  messageStream << EndOfMessageToken;

  return messageStream.str();
}
