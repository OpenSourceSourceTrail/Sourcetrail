#include <string>
#include <regex>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utilityString.h"

using namespace std::string_literals;
using namespace testing;

TEST(Split, splitWithCharDelimiter) {
  const auto result = utility::split("A,B,C", ',');

  ASSERT_EQ(3, result.size());
  EXPECT_THAT(result, Eq(std::deque<std::string>{"A", "B", "C"}));
}

TEST(Split, splitWithStringDelimiter) {
  const auto result = utility::split("A->B>C", "->");

  ASSERT_EQ(2, result.size());
  EXPECT_THAT(result, Eq(std::deque<std::string>{"A", "B>C"}));
}

TEST(Split, splitOnEmptyString) {
  const auto result = utility::split("", "->");

  ASSERT_EQ(1, result.size());
  EXPECT_THAT(result.at(0), IsEmpty());
}

TEST(Split, splitWithUnusedDelimiter) {
  const auto result = utility::split("A:B:C", ";");

  ASSERT_EQ(1, result.size());
  EXPECT_THAT(result.at(0), StrEq("A:B:C"));
}

TEST(Split, splitWithDelimitersNextToEach) {
  const auto result = utility::split("A::B:C", ':');

  ASSERT_EQ(4, result.size());
  EXPECT_THAT(result, Eq(std::deque<std::string>{"A", "", "B", "C"}));
}

TEST(Split, splitWithDelimiterAtStart) {
  const auto result = utility::split(":B:C", ':');

  ASSERT_EQ(3, result.size());
  EXPECT_THAT(result, Eq(std::deque<std::string>{"", "B", "C"}));
}

TEST(Split, splitWithDelimiterAtEnd) {
  const auto result = utility::split("B:C:", ':');

  ASSERT_EQ(3, result.size());
  ASSERT_THAT(result, Eq(std::deque<std::string>{"B", "C", ""}));
}

TEST(Join, joinWithCharDelimiter) {
  std::deque<std::string> list;
  list.push_back("A");
  list.push_back("B");
  list.push_back("C");

  std::string result = utility::join(list, ',');
  EXPECT_THAT(result, StrEq("A,B,C"));
}

TEST(Join, joinWithStringDelimiter) {
  std::deque<std::string> list;
  list.push_back("A");
  list.push_back("B");
  list.push_back("C");

  std::string result = utility::join(list, "==");
  EXPECT_THAT(result, StrEq("A==B==C"));
}

TEST(Join, joinOnEmptyList) {
  std::deque<std::string> list;
  std::string result = utility::join(list, ',');

  EXPECT_THAT(result, IsEmpty());
}

TEST(Join, joinWithEmptyStringsInList) {
  std::deque<std::string> list;
  list.push_back("A");
  list.push_back("");
  list.push_back("");

  const std::string result = utility::join(list, ':');
  EXPECT_THAT(result, StrEq("A::"));
}

TEST(Tokenize, tokenizeWithString) {
  const auto result = utility::tokenize("A->B->C", "->");

  ASSERT_EQ(5, result.size());
  EXPECT_THAT(result, Eq(std::deque<std::string>{"A", "->", "B", "->", "C"}));
}

TEST(Tokenize, tokenizeWithStringAndDelimiterAtStart) {
  const auto result = utility::tokenize("->B", "->");

  ASSERT_EQ(2, result.size());
  EXPECT_THAT(result, Eq(std::deque<std::string>{"->", "B"}));
}

TEST(Tokenize, tokenizeWithStringAndDelimiterAtEnd) {
  const auto result = utility::tokenize("C+", '+');

  ASSERT_EQ(2, result.size());
  EXPECT_THAT(result, Eq(std::deque<std::string>{"C", "+"}));
}

TEST(Tokenize, tokenizeWithDeque) {
  auto result = utility::tokenize("A->B=C->D", "->");
  result = utility::tokenize(result, "=");

  ASSERT_EQ(7, result.size());
  EXPECT_THAT(result, Eq(std::deque<std::string>{"A", "->", "B", "=", "C", "->", "D"}));
}

TEST(Substr, substrBeforeFirstWithSingleDelimiterOccurrence) {
  EXPECT_THAT(utility::substrBeforeFirst("foo bar", ' '), StrEq("foo"));
}

TEST(Substr, substrBeforeFirstWithMultipleDelimiterOccurrences) {
  EXPECT_THAT(utility::substrBeforeFirst("foo bar foo", ' '), StrEq("foo"));
}

TEST(Substr, substrBeforeFirstWithNoDelimiterOccurrence) {
  EXPECT_THAT(utility::substrBeforeFirst("foobar", ' '), StrEq("foobar"));
}

TEST(Substr, substrBeforeFirstWithDelimiterAtStart) {
  EXPECT_THAT(utility::substrBeforeFirst(" foobar", ' '), IsEmpty());
}

TEST(Substr, substrBeforeFirstWithDelimiterAtEnd) {
  EXPECT_THAT(utility::substrBeforeFirst("foobar ", ' '), StrEq("foobar"));
}

TEST(Substr, substrBeforeLastWithSingleDelimiterOccurrence) {
  EXPECT_THAT(utility::substrBeforeLast("foo bar", ' '), StrEq("foo"));
}

TEST(Substr, substrBeforeLastWithMultipleDelimiterOccurrences) {
  EXPECT_THAT(utility::substrBeforeLast("foo bar foo", ' '), StrEq("foo bar"));
}

TEST(Substr, substrBeforeLastWithNoDelimiterOccurrence) {
  EXPECT_THAT(utility::substrBeforeLast("foobar", ' '), StrEq("foobar"));
}

TEST(Substr, substrBeforeLastWithDelimiterAtStart) {
  EXPECT_THAT(utility::substrBeforeLast(" foobar", ' '), IsEmpty());
}

TEST(Substr, substrBeforeLastWithDelimiterAtEnd) {
  EXPECT_THAT(utility::substrBeforeLast("foobar ", ' '), StrEq("foobar"));
}

TEST(Substr, substrAfterWithSingleDelimiterOccurrence) {
  EXPECT_THAT(utility::substrAfter("foo bar", ' '), StrEq("bar"));
}

TEST(Substr, substrAfterWithMultipleDelimiterOccurrences) {
  EXPECT_THAT(utility::substrAfter("foo bar foo", ' '), StrEq("bar foo"));
}

TEST(Substr, substrAfterWithNoDelimiterOccurrence) {
  EXPECT_THAT(utility::substrAfter("foobar", ' '), StrEq("foobar"));
}

TEST(Substr, substrAfterWithDelimiterAtStart) {
  EXPECT_THAT(utility::substrAfter(" foobar", ' '), StrEq("foobar"));
}

TEST(Substr, substrAfterWithDelimiterAtEnd) {
  EXPECT_THAT(utility::substrAfter("foobar ", ' '), IsEmpty());
}

TEST(IsPrefix, emptyStringIsDetectedAsPrefixOfAnyOtherString) {
  const std::string foo = "foo";

  EXPECT_TRUE(utility::isPrefix<std::string>("", foo));
}

TEST(IsPrefix, prefixOfBiggerTextIsDetectedAsPrefix) {
  const std::string foobar = "foobar";
  const std::string foo = "foo";

  EXPECT_TRUE(utility::isPrefix(foo, foobar));
}

TEST(IsPrefix, prefixIsDetectedAsPrefixOfSelf) {
  const std::string foo = "foo";

  EXPECT_TRUE(utility::isPrefix(foo, foo));
}

TEST(IsPrefix, differentTextsAreNotDetectedOfPrefixesOfEachOther) {
  const std::string foo = "foo";
  const std::string bar = "bar";

  EXPECT_FALSE(utility::isPrefix(foo, bar));
  EXPECT_FALSE(utility::isPrefix(bar, foo));
}

TEST(ToLowerCase, goodCase) {
  EXPECT_THAT(utility::toLowerCase("FooBar"), StrEq("foobar"));
  EXPECT_THAT(utility::toLowerCase("FOOBAR"), StrEq("foobar"));
  EXPECT_THAT(utility::toLowerCase("foobar"), StrEq("foobar"));
}

TEST(EqualsCaseInsensitive, differentCases) {
  const std::string foo = "FooBar";
  const std::string foo2 = "foobar";

  EXPECT_TRUE(utility::equalsCaseInsensitive(foo, foo2));
}

TEST(EqualsCaseInsensitive, sameCases) {
  const std::string foo = "foobar";
  const std::string foo2 = "foobar";

  EXPECT_TRUE(utility::equalsCaseInsensitive(foo, foo2));
}

TEST(EqualsCaseInsensitive, differentStrings) {
  const std::string foo = "foo";
  const std::string foo2 = "foobar";

  EXPECT_TRUE(!utility::equalsCaseInsensitive(foo, foo2));
}

TEST(Replace, goodCase) {
  EXPECT_THAT(utility::replace("foobar", "oo", "u"), StrEq("fubar"));
  EXPECT_THAT(utility::replace("foobar", "o", "uu"), StrEq("fuuuubar"));
  EXPECT_THAT(utility::replace("foobar", "foo", ""), StrEq("bar"));
  EXPECT_THAT(utility::replace("foobar", "", "i"), StrEq("foobar"));
  EXPECT_THAT(utility::replace("foobar", "", ""), StrEq("foobar"));
  EXPECT_THAT(utility::replace("", "foo", "bar"), IsEmpty());
  EXPECT_THAT(utility::replace("foobar", "ba", "ba"), StrEq("foobar"));
}

TEST(CaseInsensitiveLess, shouldReturnFalseWhenComparingEmptyWStrings) {
  EXPECT_FALSE(utility::caseInsensitiveLess(L"", L""));
}

TEST(CaseInsensitiveLess, shouldReturnFalseWhenBothWStringsAreEqual) {
  EXPECT_FALSE(utility::caseInsensitiveLess(L"ab_cd!", L"ab_cd!"));
}

TEST(CaseInsensitiveLess, shouldReturnFalseWhenBothWStringsHaveDifferentCasesButAfterLowerCasingAreEqual) {
  EXPECT_FALSE(utility::caseInsensitiveLess(L"ab_CD!", L"aB_cD!"));
}

TEST(CaseInsensitiveLess, shouldReturnTrueWhenFirstWStringIsEmptyAndSecondNot) {
  EXPECT_TRUE(utility::caseInsensitiveLess(L"", L"ab"));
}

TEST(CaseInsensitiveLess, shouldReturnFalseWhenSecondWStringIsEmptyAndFirstNot) {
  EXPECT_FALSE(utility::caseInsensitiveLess(L"ab", L""));
}

TEST(CaseInsensitiveLess, shouldReturnTrueWhenFirstWStringIsPrefixOfSecond) {
  EXPECT_TRUE(utility::caseInsensitiveLess(L"ab_cd!", L"ab_cd!e"));
}

TEST(CaseInsensitiveLess, shouldReturnFalseWhenSecondWStringIsPrefixOfFirst) {
  EXPECT_FALSE(utility::caseInsensitiveLess(L"ab_cd!e", L"ab_cd!"));
}

TEST(CaseInsensitiveLess, shouldReturnTrueWhenAfterLowerCasingFirstWStringFirstIsPrefixOfSecond) {
  EXPECT_TRUE(utility::caseInsensitiveLess(L"aB_cd!", L"ab_cd!e"));
}

TEST(CaseInsensitiveLess, shouldReturnTrueWhenAfterLowerCasingSecondWStringFirsIsPrefixOfSecond) {
  EXPECT_TRUE(utility::caseInsensitiveLess(L"ab_cd!", L"ab_cD!e"));
}

TEST(CaseInsensitiveLess, shouldReturnTrueWhenAfterLowerCasingBothWStringsFirstIsPrefixOfSecond) {
  EXPECT_TRUE(utility::caseInsensitiveLess(L"aB_cd!", L"ab_cD!E"));
}

TEST(CaseInsensitiveLess, shouldReturnFalseWhenAfterLowerCasingFirstWStringSecondIsPrefixOfFirst) {
  EXPECT_FALSE(utility::caseInsensitiveLess(L"ab_Cd!e", L"ab_cd!"));
}

TEST(CaseInsensitiveLess, shouldReturnFalseWhenAfterLowerCasingSecondWStringSecondIsPrefixOfFirst) {
  EXPECT_FALSE(utility::caseInsensitiveLess(L"ab_cd!e", L"Ab_cd!"));
}

TEST(CaseInsensitiveLess, shouldReturnFalseWhenAfterLowerCasingBothSecondWStringSecondIsPrefixOfFirst) {
  EXPECT_FALSE(utility::caseInsensitiveLess(L"ab_cD!E", L"aB_cd!"));
}

TEST(GenerateRandomString, goodCase) {
  EXPECT_THAT(utility::createRandomString(0), IsEmpty());

  constexpr auto StringLength0 = 16U;
  const auto result0 = utility::createRandomString(StringLength0);
  ASSERT_EQ(StringLength0, result0.size());
  EXPECT_TRUE(std::regex_match(result0, std::regex("([A-z]|[0-9]){16}"s)));

  constexpr auto StringLength1 = 32U;
  const auto result1 = utility::createRandomString(StringLength1);
  ASSERT_EQ(StringLength1, result1.size());
  EXPECT_TRUE(std::regex_match(result1, std::regex("([A-z]|[0-9]){32}"s)));
}
