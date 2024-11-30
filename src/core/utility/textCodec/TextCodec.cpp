#include "TextCodec.h"

#include <QTextCodec>

TextCodec::TextCodec(std::string name) : mName(std::move(name)) {
  mCodec = QTextCodec::codecForName(mName.c_str());
  if(nullptr != mCodec) {
    mDecoder = std::make_shared<QTextDecoder>(mCodec);
    mEncoder = std::make_shared<QTextEncoder>(mCodec);
  }
}

std::string TextCodec::getName() const {
  return mName;
}

bool TextCodec::isValid() const {
  if(mCodec) {
    return true;
  }
  return false;
}

std::wstring TextCodec::decode(const std::string& unicodeString) const {
  if(mDecoder) {
    return mDecoder->toUnicode(unicodeString.c_str()).toStdWString();
  }
  return QString::fromStdString(unicodeString).toStdWString();
}

std::string TextCodec::encode(const std::wstring& string) const {
  if(mEncoder) {
    return mEncoder->fromUnicode(QString::fromStdWString(string)).toStdString();
  }
  return QString::fromStdWString(string).toStdString();
}
