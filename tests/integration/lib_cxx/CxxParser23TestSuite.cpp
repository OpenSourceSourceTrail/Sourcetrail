/**
 * @file CxxParser23TestSuite.cpp
 * @author Ahmed Abdelaal (eng.ahmedhussein89@gmail.com)
 * @brief Test the CxxParser with C++23 features
 * @version 0.1
 * @date 2026-08-26
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "CxxParserStdTestHelper.hpp"
#include "TestStorage.h"

namespace {

/**
 * https://en.cppreference.com/w/cpp/compiler_support/23
 * https://en.cppreference.com/w/cpp/23
 */

using namespace std::string_literals;

std::shared_ptr<TestStorage> parseCode(const std::string& code, const std::vector<std::wstring>& compilerFlags = {}) {
  return cxx_test::parseCode(code, L"-std=c++23", compilerFlags);
}

struct CxxParser23TestSuite : cxx_test::CxxParserStdTest {};

/**
 * Deducing this
 * https://en.cppreference.com/w/cpp/language/member_functions#Explicit_object_parameter
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html
 */
TEST_F(CxxParser23TestSuite, explicitObjectParameter) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Foo {
  int value;
  int getValue(this const Foo& self) { return self.value; }
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  // The explicit object parameter is recorded like an ordinary first parameter; the `this`
  // introducer is not part of the signature the indexer builds.
  EXPECT_THAT(client->methods, testing::Contains(L"public int Foo::getValue(const Foo &) <3:3 <3:3 <3:7 3:14> 3:36> 3:59>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/if#Consteval_if
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1938r3.html
 */
TEST_F(CxxParser23TestSuite, ifConsteval) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(constexpr int func() {
  if consteval {
    return 1;
  } else {
    return 0;
  }
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int func() <1:1 <1:1 <1:15 1:18> 1:20> 7:1>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/operators#Array_subscript_operator
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2128r6.pdf
 */
TEST_F(CxxParser23TestSuite, multidimensionalSubscriptOperator) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Matrix {
  int values[4];
  int& operator[](int row, int column) { return values[row * 2 + column]; }
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->methods, testing::Contains(L"public int & Matrix::operator[](int, int) <3:3 <3:3 <3:8 3:17> 3:38> 3:75>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/operators#Function_call_operator
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1169r4.html
 */
TEST_F(CxxParser23TestSuite, staticCallAndSubscriptOperators) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Foo {
  static int operator()(int value) { return value; }
  static int operator[](int value) { return value; }
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->methods, testing::Contains(L"public static int Foo::operator()(int) <2:3 <2:3 <2:14 2:23> 2:34> 2:52>"));
  EXPECT_THAT(client->methods, testing::Contains(L"public static int Foo::operator[](int) <3:3 <3:3 <3:14 3:23> 3:34> 3:52>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/explicit_cast
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0849r8.html
 */
TEST_F(CxxParser23TestSuite, autoDecayCopy) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(int func(const int& value) {
  auto copy = auto(value);
  return copy;
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:8> <2:8 2:11>"));
}

/**
 * https://en.cppreference.com/w/cpp/preprocessor/conditional
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2334r1.pdf
 */
TEST_F(CxxParser23TestSuite, elifdefAndElifndef) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(#define FOO 1
#ifdef BAR
int bar;
#elifdef FOO
int foo;
#else
int neither;
#endif)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int foo <5:5 5:7>"));
}

/**
 * https://en.cppreference.com/w/cpp/statements/label
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2324r2.pdf
 */
TEST_F(CxxParser23TestSuite, labelAtTheEndOfCompoundStatement) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(void func() {
  goto done;
done:
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"void func() <1:1 <1:1 <1:6 1:9> 1:11> 4:1>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/lambda
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2173r1.pdf
 */
TEST_F(CxxParser23TestSuite, attributesOnLambdas) {
  const std::shared_ptr<TestStorage> client = parseCode("auto func = [][[nodiscard]](int value) { return value; };");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"lambda at 1:13 func <1:6 1:9>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/attributes/assume
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1774r8.pdf
 */
TEST_F(CxxParser23TestSuite, assumeAttribute) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(int divide(int value) {
  [[assume(value > 0)]];
  return 100 / value;
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int divide(int) <1:1 <1:1 <1:5 1:10> 1:21> 4:1>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/implicit_conversion#Contextual_conversions
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1401r5.html
 */
TEST_F(CxxParser23TestSuite, narrowingContextualConversionsToBool) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<int N> struct Holder {
  static_assert(N & 1);
};
Holder<3> holder;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"Holder<3> holder <4:11 4:16>"));
}

/**
 * `goto`, labels and non-literal variables inside a constexpr function
 * https://en.cppreference.com/w/cpp/language/constexpr
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2242r3.html
 */
TEST_F(CxxParser23TestSuite, nonLiteralVariablesAndGotoInConstexprFunctions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(constexpr int func(bool condition) {
  if (!condition) {
    goto done;
  }
  return 1;
done:
  return 0;
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int func(bool) <1:1 <1:1 <1:15 1:18> 1:34> 8:1>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/constexpr
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2647r1.html
 */
TEST_F(CxxParser23TestSuite, staticConstexprVariablesInConstexprFunctions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(constexpr char firstDigit(int index) {
  static constexpr char digits[] = "0123456789";
  return digits[index];
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"char firstDigit(int) <1:1 <1:1 <1:16 1:25> 1:36> 4:1>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/integer_literal
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0330r8.html
 */
TEST_F(CxxParser23TestSuite, sizeTLiteralSuffix) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(auto unsignedSize = 10uz;
auto signedSize = 10z;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"__size_t unsignedSize <1:6 1:17>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"__signed_size_t signedSize <2:6 2:15>"));
}

/**
 * TODO: Add a test for the following feature
 * `<stdfloat>` only declares `std::floatN_t` when the compiler defines `__STDCPP_FLOAT32_T__`,
 * which the Clang the indexer embeds does not, so such a snippet fails to parse on this toolchain.
 *
 * https://en.cppreference.com/w/cpp/language/types#Extended_floating-point_types
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1467r9.html
 */
TEST_F(CxxParser23TestSuite, extendedFloatingPointTypes) {}

/**
 * https://en.cppreference.com/w/cpp/language/statements#Init-statement
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2360r0.html
 */
TEST_F(CxxParser23TestSuite, aliasDeclarationInInitStatement) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(int func(int value) {
  for (using Integer = int; Integer i = 0; ++i) {
    if (i == value) {
      return i;
    }
  }
  return 0;
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->typedefs, testing::Contains(L"func::Integer <2:14 2:20>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/escape
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2071r2.html
 */
TEST_F(CxxParser23TestSuite, namedUniversalCharacterEscapes) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(const char* text = "\N{LATIN SMALL LETTER A}";)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"const char * text <1:13 1:16>"));
}

/**
 * TODO: Add a test for the following feature
 * `#warning` is a diagnostic directive; the indexer records diagnostics as errors only.
 *
 * https://en.cppreference.com/w/cpp/preprocessor/error
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2437r1.pdf
 */
TEST_F(CxxParser23TestSuite, warningDirective) {}

/**
 * TODO: Add a test for the following feature
 *
 * https://en.cppreference.com/w/cpp/language/lifetime#Temporary_object_lifetime
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2718r0.html
 */
TEST_F(CxxParser23TestSuite, lifetimeExtensionInRangeBasedFor) {}
}    // namespace
