#include "NameHierarchy.h"

#include <sstream>

#include "logging.h"
#include "utilityString.h"

namespace {
constexpr std::wstring_view META_DELIMITER = L"\tm";
constexpr std::wstring_view NAME_DELIMITER = L"\tn";
constexpr std::wstring_view PART_DELIMITER = L"\ts";
constexpr std::wstring_view SIGNATURE_DELIMITER = L"\tp";
}    // namespace

std::wstring NameHierarchy::serialize(const NameHierarchy& nameHierarchy) {
  return serializeRange(nameHierarchy, 0, nameHierarchy.size());
}

std::wstring NameHierarchy::serializeRange(const NameHierarchy& nameHierarchy, size_t first, size_t last) {
  std::wstringstream stringStream;
  stringStream << nameHierarchy.getDelimiter();
  stringStream << META_DELIMITER;
  for(size_t i = first; i < last && i < nameHierarchy.size(); i++) {
    if(i > 0) {
      stringStream << NAME_DELIMITER;
    }

    stringStream << nameHierarchy[i].getName() << PART_DELIMITER;
    stringStream << nameHierarchy[i].getSignature().getPrefix();
    stringStream << SIGNATURE_DELIMITER;
    stringStream << nameHierarchy[i].getSignature().getPostfix();
  }
  return stringStream.str();
}

NameHierarchy NameHierarchy::deserialize(const std::wstring& serializedName) {
  const size_t mpos = serializedName.find(META_DELIMITER);
  if(mpos == std::wstring::npos) {
    LOG_ERROR(L"unable to deserialize name hierarchy: {}", serializedName);    // todo: obfuscate
                                                                               // serializedName!
    return NameHierarchy{NAME_DELIMITER_UNKNOWN};
  }

  NameHierarchy nameHierarchy(serializedName.substr(0, mpos));

  size_t npos = mpos + META_DELIMITER.size();
  while(npos != std::wstring::npos && npos < serializedName.size()) {
    // name
    size_t spos = serializedName.find(PART_DELIMITER, npos);
    if(spos == std::wstring::npos) {
      LOG_ERROR(L"unable to deserialize name hierarchy: {}", serializedName);    // todo: obfuscate serializedName!
      return NameHierarchy{NAME_DELIMITER_UNKNOWN};
    }

    std::wstring name = serializedName.substr(npos, spos - npos);
    spos += PART_DELIMITER.size();

    // signature
    size_t ppos = serializedName.find(SIGNATURE_DELIMITER, spos);
    if(ppos == std::wstring::npos) {
      LOG_ERROR(L"unable to deserialize name hierarchy: {}", serializedName);    // todo: obfuscate serializedName!
      return NameHierarchy{NAME_DELIMITER_UNKNOWN};
    }

    std::wstring prefix = serializedName.substr(spos, ppos - spos);
    ppos += SIGNATURE_DELIMITER.size();

    std::wstring postfix;
    npos = serializedName.find(NAME_DELIMITER, ppos);
    if(npos == std::wstring::npos) {
      postfix = serializedName.substr(ppos, std::wstring::npos);
    } else {
      postfix = serializedName.substr(ppos, npos - ppos);
      npos += NAME_DELIMITER.size();
    }

    nameHierarchy.push(NameElement(std::move(name), std::move(prefix), std::move(postfix)));
  }

  // TODO: replace duplicate main definition fix with better solution
  if(nameHierarchy.size() == 1 && nameHierarchy.back().hasSignature() && !nameHierarchy.back().getName().empty() &&
     nameHierarchy.back().getName()[0] == '.' && utility::isPrefix<std::wstring>(L".:main:.", nameHierarchy.back().getName())) {
    const NameElement::Signature sig = nameHierarchy.back().getSignature();
    nameHierarchy.pop();
    nameHierarchy.push(NameElement(L"main", sig.getPrefix(), sig.getPostfix()));
  }

  return nameHierarchy;
}

const std::wstring& NameHierarchy::getDelimiter() const {
  return mDelimiter;
}

void NameHierarchy::setDelimiter(std::wstring delimiter) {
  mDelimiter = std::move(delimiter);
}

NameHierarchy::NameHierarchy(std::wstring delimiter) : mDelimiter(std::move(delimiter)) {}

NameHierarchy::NameHierarchy(const std::vector<std::wstring>& names, std::wstring delimiter) : mDelimiter(std::move(delimiter)) {
  for(const std::wstring& name : names) {
    push(name);
  }
}

NameHierarchy::NameHierarchy(std::wstring name, std::wstring delimiter) : mDelimiter(std::move(delimiter)) {
  push(std::move(name));
}

NameHierarchy::NameHierarchy(NameDelimiterType delimiterType) : NameHierarchy(nameDelimiterTypeToString(delimiterType)) {}

NameHierarchy::NameHierarchy(std::wstring name, NameDelimiterType delimiterType)
    : NameHierarchy(std::move(name), nameDelimiterTypeToString(delimiterType)) {}

NameHierarchy::NameHierarchy(const std::vector<std::wstring>& names, NameDelimiterType delimiterType)
    : NameHierarchy(names, nameDelimiterTypeToString(delimiterType)) {}

NameHierarchy::NameHierarchy(const NameHierarchy& other) = default;

NameHierarchy& NameHierarchy::operator=(const NameHierarchy& other) = default;

NameHierarchy::NameHierarchy(NameHierarchy&& other) noexcept
    : mElements(std::move(other.mElements)), mDelimiter(std::move(other.mDelimiter)) {}

NameHierarchy& NameHierarchy::operator=(NameHierarchy&& other) noexcept {
  mElements = std::move(other.mElements);
  mDelimiter = std::move(other.mDelimiter);
  return *this;
}

NameHierarchy::~NameHierarchy() = default;

void NameHierarchy::push(const NameElement& element) {
  mElements.emplace_back(element);
}

void NameHierarchy::push(std::wstring name) {
  mElements.emplace_back(std::move(name));
}

void NameHierarchy::pop() {
  mElements.pop_back();
}

NameElement& NameHierarchy::back() {
  return mElements.back();
}

const NameElement& NameHierarchy::back() const {
  return mElements.back();
}

NameElement& NameHierarchy::operator[](size_t pos) {
  return mElements[pos];
}

const NameElement& NameHierarchy::operator[](size_t pos) const {
  return mElements[pos];
}

NameHierarchy NameHierarchy::getRange(size_t first, size_t last) const {
  NameHierarchy hierarchy(mDelimiter);

  for(size_t i = first; i < last; i++) {
    hierarchy.push(mElements[i]);
  }

  return hierarchy;
}

size_t NameHierarchy::size() const {
  return mElements.size();
}

std::wstring NameHierarchy::getQualifiedName() const {
  std::wstringstream stringStream;
  for(size_t i = 0; i < mElements.size(); i++) {
    if(i > 0) {
      stringStream << mDelimiter;
    }
    stringStream << mElements[i].getName();
  }
  return stringStream.str();
}

std::wstring NameHierarchy::getQualifiedNameWithSignature() const {
  std::wstring name = getQualifiedName();
  if(!mElements.empty()) {
    name = mElements.back().getSignature().qualifyName(name);    // todo: use separator for signature!
  }
  return name;
}

std::wstring NameHierarchy::getRawName() const {
  if(!mElements.empty()) {
    return mElements.back().getName();
  }
  return L"";
}

std::wstring NameHierarchy::getRawNameWithSignature() const {
  if(!mElements.empty()) {
    return mElements.back().getNameWithSignature();
  }
  return L"";
}

std::wstring NameHierarchy::getRawNameWithSignatureParameters() const {
  if(!mElements.empty()) {
    return mElements.back().getNameWithSignatureParameters();
  }
  return L"";
}

bool NameHierarchy::hasSignature() const {
  if(!mElements.empty()) {
    return mElements.back().hasSignature();
  }

  return false;
}

NameElement::Signature NameHierarchy::getSignature() const {
  if(!mElements.empty()) {
    return mElements.back().getSignature();    // todo: use separator for signature!
  }

  return {};
}
