#pragma once
#include <memory>
#include <string>

class QStringDecoder;
class QStringEncoder;

class TextCodec final {
public:
  explicit TextCodec(std::string name);

  [[nodiscard]] std::string getName() const;
  [[nodiscard]] bool isValid() const;

  [[nodiscard]] std::wstring decode(const std::string& unicodeString) const;

  [[nodiscard]] std::string encode(const std::wstring& string) const;

private:
  const std::string mName;
  mutable std::shared_ptr<QStringDecoder> mDecoder;
  mutable std::shared_ptr<QStringEncoder> mEncoder;
};
