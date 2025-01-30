#include "Version.h"

#include <cstdint>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <range/v3/view/split.hpp>
#include <range/v3/view/transform.hpp>

#include "logging.h"
#include "utilityString.h"

Version Version::sVersion;

Version Version::fromString(const std::string& versionString) {
  auto partView = versionString | ranges::views::split('.') |
      ranges::views::transform([](auto&& value) { return static_cast<uint32_t>(std::stoi(&*value.begin())); });
  std::vector<uint32_t> parts(partView.begin(), partView.end());
  if(parts.empty() || parts.size() != 3) {
    LOG_WARNING(fmt::format("Version string is invalid: {}", versionString));
    return Version{};
  }

  return Version{parts[0], parts[1], parts[2]};
}

void Version::setApplicationVersion(const Version& version) {
  sVersion = version;
}

const Version& Version::getApplicationVersion() {
  return sVersion;
}

Version::Version(uint32_t major, uint32_t minor, uint32_t patch) noexcept : mMajor(major), mMinor(minor), mPatch(patch) {}

bool Version::isEmpty() const {
  return mMajor == 0 && mMinor == 0 && mPatch == 0;
}

bool Version::isValid() const {
  return (mMajor >= 1U) && (mMajor <= 99U) && (mMinor <= 99U) && (mPatch <= 99U);
}

std::string Version::toString() const {
  return fmt::format("{}.{}.{}", mMajor, mMinor, mPatch);
}

std::wstring Version::toWString() const {
  return utility::decodeFromUtf8(fmt::format("{}.{}.{}", mMajor, mMinor, mPatch));
}

bool Version::operator<(const Version& other) const {
  if(mMajor != other.mMajor) {
    return mMajor < other.mMajor;
  } else if(mMinor != other.mMinor) {
    return mMinor < other.mMinor;
  } else {
    return mPatch < other.mPatch;
  }
}

bool Version::operator>(const Version& other) const {
  if(mMajor != other.mMajor) {
    return mMajor > other.mMajor;
  } else if(mMinor != other.mMinor) {
    return mMinor > other.mMinor;
  } else {
    return mPatch > other.mPatch;
  }
}

bool Version::operator==(const Version& other) const {
  return mMajor == other.mMajor && mMinor == other.mMinor && mPatch == other.mPatch;
}