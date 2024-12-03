#include <sstream>

#include <gtest/gtest.h>

#include "NetworkProtocolHelper.h"

TEST(NetworkProtocolHelper, parseMessage) {
  std::wstring type = L"setActiveToken";
  std::wstring divider = L">>";
  std::wstring filePath = L"C:/Users/Manuel/important/file/location/fileName.cpp";
  std::wstring endOfMessageToken = L"<EOM>";
  int row = 1;
  int column = 2;

  // valid message
  std::wstringstream message;
  message << type << divider << filePath << divider << row << divider << column << endOfMessageToken;

  NetworkProtocolHelper::SetActiveTokenMessage networkMessage = NetworkProtocolHelper::parseSetActiveTokenMessage(message.str());

  EXPECT_TRUE(networkMessage.filePath.wstr() == filePath);
  EXPECT_TRUE(networkMessage.row == row);
  EXPECT_TRUE(networkMessage.column == column);
  EXPECT_TRUE(networkMessage.valid == true);

  // invalid type
  message.str(L"");
  message << L"foo" << divider << filePath << divider << row << divider << column << endOfMessageToken;
  networkMessage = NetworkProtocolHelper::parseSetActiveTokenMessage(message.str());

  EXPECT_TRUE(networkMessage.filePath.wstr() == L"");
  EXPECT_TRUE(networkMessage.row == 0);
  EXPECT_TRUE(networkMessage.column == 0);
  EXPECT_TRUE(networkMessage.valid == false);

  // missing divider
  message.str(L"");
  message << type << divider << filePath << row << divider << column << endOfMessageToken;
  networkMessage = NetworkProtocolHelper::parseSetActiveTokenMessage(message.str());

  EXPECT_TRUE(networkMessage.filePath.wstr() == L"");
  EXPECT_TRUE(networkMessage.row == 0);
  EXPECT_TRUE(networkMessage.column == 0);
  EXPECT_TRUE(networkMessage.valid == false);

  // invalid row
  message.str(L"");
  message << type << divider << filePath << divider << "potato" << divider << column << endOfMessageToken;
  networkMessage = NetworkProtocolHelper::parseSetActiveTokenMessage(message.str());

  EXPECT_TRUE(networkMessage.filePath.wstr() == L"");
  EXPECT_TRUE(networkMessage.row == 0);
  EXPECT_TRUE(networkMessage.column == 0);
  EXPECT_TRUE(networkMessage.valid == false);

  // invalid column
  message.str(L"");
  message << type << divider << filePath << divider << row << divider << "laz0r" << endOfMessageToken;
  networkMessage = NetworkProtocolHelper::parseSetActiveTokenMessage(message.str());

  EXPECT_TRUE(networkMessage.filePath.wstr() == L"");
  EXPECT_TRUE(networkMessage.row == 0);
  EXPECT_TRUE(networkMessage.column == 0);
  EXPECT_TRUE(networkMessage.valid == false);
}
