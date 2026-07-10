#pragma once
#include <string>

class TextCodec final {
public:
  explicit TextCodec(std::string name);

  [[nodiscard]] std::string getName() const;
  [[nodiscard]] bool isValid() const;

  [[nodiscard]] std::wstring decode(const std::string& unicodeString) const;

  [[nodiscard]] std::string encode(const std::wstring& string) const;

private:
  const std::string mName;
  const bool mValid;
};
