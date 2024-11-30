#pragma once
#include <cstdint>
#include <string>

class Version final {
public:
  static Version fromString(const std::string& versionString);

  static void setApplicationVersion(const Version& version);
  static const Version& getApplicationVersion();

  explicit Version(uint32_t major = 0, uint32_t minor = 0, uint32_t patch = 0) noexcept;

  [[nodiscard]] bool isEmpty() const;
  [[nodiscard]] bool isValid() const;

  [[nodiscard]] std::string toString() const;
  [[nodiscard]] std::wstring toWString() const;

  bool operator<(const Version& other) const;
  bool operator>(const Version& other) const;
  bool operator==(const Version& other) const;

private:
  static Version sVersion;

  uint32_t mMajor;
  uint32_t mMinor;
  uint32_t mPatch;
};