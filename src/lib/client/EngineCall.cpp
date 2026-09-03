#include "EngineCall.h"

namespace client {

namespace {
LocalDispatch& dispatchSlot() {
  static LocalDispatch sDispatch;
  return sDispatch;
}
}    // namespace

void setLocalDispatch(LocalDispatch dispatch) {
  dispatchSlot() = std::move(dispatch);
}

const LocalDispatch& localDispatch() {
  return dispatchSlot();
}

std::string urlEncode(const std::string& text) {
  static constexpr char Hex[] = "0123456789ABCDEF";

  std::string out;
  out.reserve(text.size());
  for(const char rawChr : text) {
    const auto chr = static_cast<unsigned char>(rawChr);
    // Unreserved per RFC 3986. Everything else -- '/' and ':' in a Windows path included -- is
    // escaped, so a file path survives as a single path segment.
    if((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') || (chr >= '0' && chr <= '9') || chr == '-' || chr == '_' ||
       chr == '.' || chr == '~') {
      out.push_back(static_cast<char>(chr));
      continue;
    }
    out.push_back('%');
    out.push_back(Hex[chr >> 4]);
    out.push_back(Hex[chr & 0x0F]);
  }
  return out;
}

std::string joinIds(const std::vector<Id>& ids) {
  std::string out;
  for(const Id id : ids) {
    if(!out.empty()) {
      out.push_back(',');
    }
    out += std::to_string(id);
  }
  return out;
}

}    // namespace client
