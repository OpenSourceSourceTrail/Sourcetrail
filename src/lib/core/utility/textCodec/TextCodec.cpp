#include "TextCodec.h"

#include <boost/locale/encoding.hpp>
#include <boost/locale/encoding_errors.hpp>
#include <boost/locale/encoding_utf.hpp>

namespace {

bool isCharsetValid(const std::string& name) {
  if(name.empty()) {
    return false;
  }
  try {
    boost::locale::conv::from_utf(std::wstring(), name);
    return true;
  } catch(const boost::locale::conv::invalid_charset_error&) {
    return false;
  }
}

}    // namespace

TextCodec::TextCodec(std::string name) : mName(std::move(name)), mValid(isCharsetValid(mName)) {}

std::string TextCodec::getName() const {
  return mName;
}

bool TextCodec::isValid() const {
  return mValid;
}

std::wstring TextCodec::decode(const std::string& unicodeString) const {
  return boost::locale::conv::utf_to_utf<wchar_t>(unicodeString);
}

std::string TextCodec::encode(const std::wstring& string) const {
  if(mValid) {
    try {
      return boost::locale::conv::from_utf(string, mName);
    } catch(const boost::locale::conv::conversion_error&) {
      // fall through to UTF-8 fallback
    }
  }
  return boost::locale::conv::utf_to_utf<char>(string);
}
