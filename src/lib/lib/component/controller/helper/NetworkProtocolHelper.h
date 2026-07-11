#pragma once
#include <string>
#include <vector>

#include "FilePath.h"

class NetworkProtocolHelper {
public:
  struct SetActiveTokenMessage {
    FilePath filePath{L""};
    unsigned int row = 0;
    unsigned int column = 0;
    bool valid = false;
  };

  struct CreateProjectMessage {};

  struct CreateCDBProjectMessage {
    FilePath cdbFileLocation{L""};
    std::wstring ideId;
    bool valid = false;
  };

  struct PingMessage {
    std::wstring ideId;
    bool valid = false;
  };

  enum MESSAGE_TYPE : uint8_t { UNKNOWN = 0, SET_ACTIVE_TOKEN, CREATE_PROJECT, CREATE_CDB_MESSAGE, PING };

  static MESSAGE_TYPE getMessageType(const std::wstring& message);

  static SetActiveTokenMessage parseSetActiveTokenMessage(const std::wstring& message);
  static CreateProjectMessage parseCreateProjectMessage(const std::wstring& message);
  static CreateCDBProjectMessage parseCreateCDBProjectMessage(const std::wstring& message);
  static PingMessage parsePingMessage(const std::wstring& message);

  static std::wstring buildSetIDECursorMessage(const FilePath& fileLocation, const unsigned int row, const unsigned int column);
  static std::wstring buildCreateCDBMessage();
  static std::wstring buildPingMessage();
};
