#include <gtest/gtest.h>

#include "FilePath.h"
#define private public
#include "CxxVs17HeaderPathDetector.hpp"
#undef private

namespace {

TEST(CxxVs17HeaderPathDetectorTestSuite, testDetectHeaderPaths) {
  CxxVs17HeaderPathDetector detector;
  std::vector<FilePath> headerPaths = detector.getPaths();

  // Basic checks
  EXPECT_FALSE(headerPaths.empty()) << "No header paths detected";

  // Check for presence of known headers
  bool foundVcruntime = false;
  bool foundWindowsSdk = false;

  for(const FilePath& path : headerPaths) {
    if(path.getConcatenated(L"vcruntime.h").exists()) {
      foundVcruntime = true;
    }
    if(path.wstr().find(L"Windows Kits") != std::wstring::npos) {
      foundWindowsSdk = true;
    }
  }

  EXPECT_TRUE(foundVcruntime) << "vcruntime.h not found in detected header paths";
  EXPECT_TRUE(foundWindowsSdk) << "Windows SDK headers not found in detected header paths";

  for(const FilePath& path : headerPaths) {
    std::wcout << L"Detected header path: " << path.wstr() << std::endl;
  }
}
}    // namespace