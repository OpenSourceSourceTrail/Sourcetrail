#include <gtest/gtest.h>

#include "FilePathFilter.h"

TEST(FilePathFilter, findsExactMatch) {
  FilePathFilter filter(L"test.h");

  EXPECT_TRUE(filter.isMatching(FilePath(L"test.h")));
}

TEST(FilePathFilter, findsMatchWithSingleAsteriskInSameLevel) {
  FilePathFilter filter(L"*test.*");

  EXPECT_TRUE(filter.isMatching(FilePath(L"this_is_a_test.h")));
}

TEST(FilePathFilter, findsMatchWithSingleAsteriskInDifferentLevel) {
  FilePathFilter filter(L"*/this_is_a_test.h");

  EXPECT_TRUE(filter.isMatching(FilePath(L"folder/this_is_a_test.h")));
}

TEST(FilePathFilter, doesNotFindMatchWithSingleAsteriskInDifferentLevel) {
  FilePathFilter filter(L"*/test.h");

  EXPECT_TRUE(!filter.isMatching(FilePath(L"test.h")));
}

TEST(FilePathFilter, findsMatchWithMultipleAsteriskInSameLevel) {
  FilePathFilter filter(L"**test.h");

  EXPECT_TRUE(filter.isMatching(FilePath(L"folder/this_is_a_test.h")));
}

TEST(FilePathFilter, findsMatchWithMultipleAsteriskInDifferentLevel) {
  FilePathFilter filter(L"root/**/test.h");

  EXPECT_TRUE(filter.isMatching(FilePath(L"root/folder1/folder2/test.h")));
}

TEST(FilePathFilter, doesNotFindMatchWithMultipleAsteriskInDifferentLevel) {
  FilePathFilter filter(L"**/test.h");

  EXPECT_TRUE(!filter.isMatching(FilePath(L"folder/this_is_a_test.h")));
}

TEST(FilePathFilter, escapesDotCharacter) {
  FilePathFilter filter(L"test.h");

  EXPECT_TRUE(!filter.isMatching(FilePath(L"testyh")));
}

TEST(FilePathFilter, escapesPlusCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder/test+.h").isMatching(FilePath(L"folder/test+.h")));
}

TEST(FilePathFilter, escapesMinusCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder/test[-].h").isMatching(FilePath(L"folder/test[-].h")));
}

TEST(FilePathFilter, escapesDollarCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder/test$.h").isMatching(FilePath(L"folder/test$.h")));
}

TEST(FilePathFilter, escapesCircumflexCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder/test^.h").isMatching(FilePath(L"folder/test^.h")));
}

TEST(FilePathFilter, escapesOpenRoundBraceCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder/test(.h").isMatching(FilePath(L"folder/test(.h")));
}

TEST(FilePathFilter, escapesCloseRoundBraceCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder\\test).h").isMatching(FilePath(L"folder/test).h")));
}

TEST(FilePathFilter, escapesOpenCurlyBraceCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder/test{.h").isMatching(FilePath(L"folder/test{.h")));
}

TEST(FilePathFilter, escapesCloseCurlyBraceCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder/test}.h").isMatching(FilePath(L"folder/test}.h")));
}

TEST(FilePathFilter, escapesOpenSquaredBraceCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder/test[.h").isMatching(FilePath(L"folder/test[.h")));
}

TEST(FilePathFilter, escapesCloseSquaredBraceCharacter) {
  EXPECT_TRUE(FilePathFilter(L"folder\\test].h").isMatching(FilePath(L"folder/test].h")));
}

TEST(FilePathFilter, findsBackslashIfSlashWasProvided) {
  FilePathFilter filter(L"folder/test.h");

  EXPECT_TRUE(filter.isMatching(FilePath(L"folder\\test.h")));
}

TEST(FilePathFilter, findsSlashIfBackslashWasProvided) {
  FilePathFilter filter(L"folder\\test.h");

  EXPECT_TRUE(filter.isMatching(FilePath(L"folder/test.h")));
}
