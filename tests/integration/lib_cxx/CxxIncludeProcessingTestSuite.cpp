#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../../src/lib/tests/mocks/MockedApplicationSetting.hpp"
#include "IncludeDirective.h"
#include "IncludeProcessing.h"
#include "TextAccess.h"
#include "language_packages.h"
#include "utility.h"

struct CxxIncludeProcessing : testing::Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mMocked);
    EXPECT_CALL(*mMocked, getLoggingEnabled).WillRepeatedly(testing::Return(false));
    ON_CALL(*mMocked, getTextEncoding).WillByDefault(testing::Return("UTF-8"));
  }
  void TearDown() override {
    IApplicationSettings::setInstance(nullptr);
    mMocked.reset();
  }
  std::shared_ptr<MockedApplicationSettings> mMocked = std::make_shared<MockedApplicationSettings>();
};

TEST_F(CxxIncludeProcessing, includeDetectionFindsIncludeWithQuotes) {
  auto includeDirectives = IncludeProcessing::getIncludeDirectives(
      TextAccess::createFromString("#include \"foo.h\"", FilePath(L"foo.cpp")));

  ASSERT_FALSE(includeDirectives.empty());
  EXPECT_THAT(includeDirectives.front().getIncludedFile().wstr(), testing::StrEq(L"foo.h"));
  EXPECT_THAT(includeDirectives.front().getIncludingFile().wstr(), testing::StrEq(L"foo.cpp"));
}

TEST_F(CxxIncludeProcessing, includeDetectionFindsIncludeWithAngleBrackets) {
  auto includeDirectives = IncludeProcessing::getIncludeDirectives(
      TextAccess::createFromString("#include <foo.h>", FilePath(L"foo.cpp")));

  ASSERT_FALSE(includeDirectives.empty());
  EXPECT_THAT(includeDirectives.front().getIncludedFile().wstr(), testing::StrEq(L"foo.h"));
  EXPECT_THAT(includeDirectives.front().getIncludingFile().wstr(), testing::StrEq(L"foo.cpp"));
}

TEST_F(CxxIncludeProcessing, includeDetectionFindsIncludeWithQuotesAndSpaceBeforeKeyword) {
  auto includeDirectives = IncludeProcessing::getIncludeDirectives(
      TextAccess::createFromString("# include \"foo.h\"", FilePath(L"foo.cpp")));

  ASSERT_FALSE(includeDirectives.empty());
  EXPECT_THAT(includeDirectives.front().getIncludedFile().wstr(), testing::StrEq(L"foo.h"));
  EXPECT_THAT(includeDirectives.front().getIncludingFile().wstr(), testing::StrEq(L"foo.cpp"));
}

TEST_F(CxxIncludeProcessing, includeDetectionDoesNotFindIncludeInEmptyFile) {
  EXPECT_TRUE(IncludeProcessing::getIncludeDirectives(TextAccess::createFromString("")).empty());
}

TEST_F(CxxIncludeProcessing, includeDetectionDoesNotFindIncludeInFileWithoutPreprocessorDirective) {
  EXPECT_TRUE(IncludeProcessing::getIncludeDirectives(TextAccess::createFromString("foo")).empty());
}

TEST_F(CxxIncludeProcessing, includeDetectionDoesNotFindIncludeInFileWithoutIncludePreprocessorDirective) {
  EXPECT_TRUE(IncludeProcessing::getIncludeDirectives(TextAccess::createFromString("#ifdef xx\n#endif")).empty());
}

TEST_F(CxxIncludeProcessing, headerSearchPathDetectionDoesNotFindPathRelativeToIncludingFile) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_does_not_find_path_relative_to_including_"
                L"file/a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_does_not_find_path_relative_to_including_"
                L"file")},
      {},
      1,
      [](float) {}));

  EXPECT_TRUE(headerSearchDirectories.empty());
}

TEST_F(CxxIncludeProcessing, headerSearchPathDetectionFindsPathInsideSubDirectory) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_inside_sub_directory/a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_inside_sub_directory")},
      {},
      1,
      [](float) {}));

  EXPECT_TRUE(
      utility::containsElement<FilePath>(headerSearchDirectories,
                                         FilePath(L"data/CxxIncludeProcessingTestSuite/"
                                                  L"test_header_search_path_detection_finds_path_inside_sub_directory/include")
                                             .makeAbsolute()));
}

TEST_F(CxxIncludeProcessing, headerSearchPathDetectionFindsPathRelativeToSubDirectory) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_relative_to_sub_directory/"
                L"a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_relative_to_sub_directory")},
      {},
      1,
      [](float) {}));

  EXPECT_TRUE(utility::containsElement<FilePath>(
      headerSearchDirectories,
      FilePath(L"data/CxxIncludeProcessingTestSuite/"
               L"test_header_search_path_detection_finds_path_relative_to_sub_directory/include")
          .makeAbsolute()));
}

TEST_F(CxxIncludeProcessing, headerSearchPathDetectionFindsPathIncludedInHeaderSearchPath) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_header_search_"
                L"path/a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_header_search_"
                L"path/include_b")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_header_search_"
                L"path/include_a")},
      1,
      [](float) {}));


  EXPECT_TRUE(
      utility::containsElement<FilePath>(headerSearchDirectories,
                                         FilePath(L"data/CxxIncludeProcessingTestSuite/"
                                                  L"test_header_search_path_detection_finds_path_included_in_header_search_path/"
                                                  L"include_b")
                                             .makeAbsolute()));
}

TEST_F(CxxIncludeProcessing, headerSearchPathDetectionFindsPathIncludedInFutureHeaderSearchPath) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_future_header_"
                L"search_path/a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_future_header_"
                L"search_path")},
      {},
      1,
      [](float) {}));

  EXPECT_TRUE(utility::containsElement<FilePath>(
      headerSearchDirectories,
      FilePath(L"data/CxxIncludeProcessingTestSuite/"
               L"test_header_search_path_detection_finds_path_included_in_future_header_search_"
               L"path/include_a")
          .makeAbsolute()));
  EXPECT_TRUE(utility::containsElement<FilePath>(
      headerSearchDirectories,
      FilePath(L"data/CxxIncludeProcessingTestSuite/"
               L"test_header_search_path_detection_finds_path_included_in_future_header_search_"
               L"path/include_b")
          .makeAbsolute()));
}