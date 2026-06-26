#include "TextCodec.h"

#include <memory>

#include <QString>
#include <QStringDecoder>
#include <QStringEncoder>

TextCodec::TextCodec(std::string name)
    : mName(std::move(name))
    , mDecoder(std::make_shared<QStringDecoder>(mName.c_str()))
    , mEncoder(std::make_shared<QStringEncoder>(mName.c_str())) {}

std::string TextCodec::getName() const {
  return mName;
}

bool TextCodec::isValid() const {
  return mDecoder->isValid() && mEncoder->isValid();
}

std::wstring TextCodec::decode(const std::string& unicodeString) const {
  return QString::fromStdString(unicodeString).toStdWString();
}

std::string TextCodec::encode(const std::wstring& string) const {
  if(mEncoder->isValid()) {
    return (*mEncoder)(QString::fromStdWString(string)).data.toStdString();
  }
  return QString::fromStdWString(string).toStdString();
}
