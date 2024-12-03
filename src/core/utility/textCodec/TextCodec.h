#pragma once
#include <memory>
#include <string>

class QTextCodec;
class QTextDecoder;
class QTextEncoder;

class TextCodec final {
public:
  explicit TextCodec(std::string name);

  [[nodiscard]] std::string getName() const;
  [[nodiscard]] bool isValid() const;

  [[nodiscard]] std::wstring decode(const std::string& unicodeString) const;

  [[nodiscard]] std::string encode(const std::wstring& string) const;

private:
  const std::string mName;
  QTextCodec* mCodec = nullptr;
  std::shared_ptr<QTextDecoder> mDecoder;
  std::shared_ptr<QTextEncoder> mEncoder;
};
