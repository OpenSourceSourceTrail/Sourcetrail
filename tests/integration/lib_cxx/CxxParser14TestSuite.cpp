/**
 * @file CxxParser17TestSuite.cpp
 * @author Ahmed Abdelaal (eng.ahmedhussein89@gmail.com)
 * @brief Test the CxxParser with C++14 features
 * @version 0.1
 * @date 2025-03-10
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "CxxParser.h"
#include "IndexerStateInfo.h"
#include "IntermediateStorage.h"
#include "MockedApplicationSetting.hpp"
#include "ParserClientImpl.h"
#include "TestFileRegister.h"
#include "TestStorage.h"
#include "TextAccess.h"
#include "utility.h"

namespace {

/**
 * https://en.cppreference.com/w/cpp/compiler_support/14
 * https://en.cppreference.com/w/cpp/14
 */

using namespace std::string_literals;

std::shared_ptr<TestStorage> parseCode(const std::string& code, const std::vector<std::wstring>& compilerFlags = {}) {
  auto storage = std::make_shared<IntermediateStorage>();

  CxxParser parser(std::make_shared<ParserClientImpl>(storage.get()),
                   std::make_shared<TestFileRegister>(),
                   std::make_shared<IndexerStateInfo>());

  parser.buildIndex(L"temp.cpp",
                    TextAccess::createFromString(code),
                    utility::concat(compilerFlags, std::vector<std::wstring>(1, L"-std=c++14")));

  return TestStorage::create(storage);
}

struct CxxParser14TestSuite : testing::Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mMocked);
    EXPECT_CALL(*mMocked, getLoggingEnabled).WillRepeatedly(testing::Return(false));
  }

  void TearDown() override {
    IApplicationSettings::setInstance(nullptr);
  }
  std::shared_ptr<MockedApplicationSettings> mMocked = std::make_shared<MockedApplicationSettings>();
};

/**
 * TODO: Add a test for the following feature
 *
 * https://en.cppreference.com/w/cpp/language/implicit_conversion#Contextual_conversions
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3323.pdf
 */
TEST_F(CxxParser14TestSuite, tweakedWordingForContextualConversions) {}

/**
 * https://en.cppreference.com/w/cpp/language/integer_literal
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3472.pdf
 */
TEST_F(CxxParser14TestSuite, binaryLiterals) {
  const std::shared_ptr<TestStorage> client = parseCode("int b = 0b101010;");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int b <1:5 1:5>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/auto
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3638.html
 */
TEST_F(CxxParser14TestSuite, returnTypeDeductionForNormalFunctions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(auto sum() { return 10; })");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int sum() <1:1 <1:1 <1:6 1:8> 1:10> 1:25>"));
}

TEST_F(CxxParser14TestSuite, decltypeAuto) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(const int x = 0;
decltype(auto) y = x;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"const int x <1:11 1:11>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"const int y <2:16 2:16>"));
}

/**
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3648.html
 */
TEST_F(CxxParser14TestSuite, initializedGeneralizedLambdaCaptures) {
  const std::shared_ptr<TestStorage> client = parseCode("auto func = [x = 10]() { return x; };");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"lambda at 1:13 func <1:6 1:9>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/lambda#Explanation
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3649.html
 */
TEST_F(CxxParser14TestSuite, genericLambdaExpressions) {
  const std::shared_ptr<TestStorage> client = parseCode("auto func = [](auto x) { return x; };");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"lambda at 1:13 func <1:6 1:9>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/variable_template
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3651.pdf
 */
TEST_F(CxxParser14TestSuite, variableTemplates) {
  const std::shared_ptr<TestStorage> client = parseCode("template<class T>constexpr T pi = T(3.1415926535897932385);");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"const T pi<class T> <1:30 1:31>"));
}

/**
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3652.html
 */
TEST_F(CxxParser14TestSuite, extendedConstexpr) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(constexpr int factorial(int n) {
  if (n <= 1) {
    return 1;
  } else {
    return n * factorial(n - 1);
  }
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int factorial(int) <1:1 <1:1 <1:15 1:23> 1:30> 7:1>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/attributes/deprecated
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3760.html
 */
TEST_F(CxxParser14TestSuite, deprecated) {
  const std::shared_ptr<TestStorage> client = parseCode("[[deprecated]] void func();");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"void func() <1:16 <1:21 1:24> 1:26>"));
}

/**
 * Single quote as digit separator
 * https://en.cppreference.com/w/cpp/language/integer_literal#Single_quote
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3781.pdf
 */
TEST_F(CxxParser14TestSuite, singleQuoteAsDigitSeparator) {
  const std::shared_ptr<TestStorage> client = parseCode("unsigned long long l2 = 18'446'744'073'709'550'592llu;");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"unsigned long long l2 <1:20 1:21>"));
}
}    // namespace
