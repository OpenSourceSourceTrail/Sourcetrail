#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IncludeDirective.h"
#include "utilityString.h"

using namespace testing;

TEST(IncludeDirective, emptyIncludedFilePath) {
  constexpr auto source =
      "data/CxxIncludeProcessingTestSuite/test_header_search_path_detection_does_not_find_path_relative_to_including_file/a.cpp";
  auto includeDirective = IncludeDirective(FilePath{}, FilePath{source}, 0U, false);
  EXPECT_THAT(includeDirective.getIncludedFile().str(), IsEmpty());
  EXPECT_THAT(includeDirective.getDirective(), IsEmpty());
  EXPECT_THAT(includeDirective.getIncludingFile().str(), StrEq(source));
  EXPECT_EQ(0U, includeDirective.getLineNumber());
}

TEST(IncludeDirective, emptyIncludingFilePath) {
  constexpr auto header = "a.h";
  auto includeDirective = IncludeDirective(FilePath{header}, FilePath{}, 0U, false);
  EXPECT_THAT(includeDirective.getIncludedFile().str(), StrEq(header));
  EXPECT_THAT(includeDirective.getDirective(), StrEq(L"#include \"a.h\""));
  EXPECT_THAT(includeDirective.getIncludingFile().str(), IsEmpty());
  EXPECT_EQ(0U, includeDirective.getLineNumber());
}

TEST(IncludeDirective, goodCase) {
  constexpr auto header = "a.h";
  constexpr auto source =
      "data/CxxIncludeProcessingTestSuite/test_header_search_path_detection_does_not_find_path_relative_to_including_file/a.cpp";
  auto includeDirective = IncludeDirective(FilePath{header}, FilePath{source}, 0U, false);
  EXPECT_THAT(includeDirective.getIncludedFile().str(), StrEq(header));
  EXPECT_THAT(utility::encodeToUtf8(includeDirective.getDirective()), StrEq("#include \"a.h\""));
  EXPECT_THAT(includeDirective.getIncludingFile().str(), StrEq(source));
  EXPECT_EQ(0, includeDirective.getLineNumber());
}