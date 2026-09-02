#pragma once
// STL
#include <string>
#include <vector>
// internal
#include "Message.h"

class MessageStatus final : public Message<MessageStatus> {
public:
  MessageStatus(const std::wstring& status_, bool isError_ = false, bool showLoader_ = false, bool showInStatusBar_ = true);

  MessageStatus(const std::vector<std::wstring>& stati_,
                bool isError_ = false,
                bool showLoader_ = false,
                bool showInStatusBar_ = true);

  static const std::string getStaticType();

  const std::vector<std::wstring>& stati() const;
  std::wstring status() const;
  virtual void print(std::wostream& os) const;

  const bool isError;
  const bool showLoader;
  const bool showInStatusBar;

private:
  std::vector<std::wstring> m_stati;
};