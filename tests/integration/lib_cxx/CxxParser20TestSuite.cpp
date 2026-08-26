/**
 * @file CxxParser20TestSuite.cpp
 * @author Ahmed Abdelaal (eng.ahmedhussein89@gmail.com)
 * @brief Test the CxxParser with C++20 features
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
 * https://en.cppreference.com/w/cpp/compiler_support/20
 * https://en.cppreference.com/w/cpp/20
 */

using namespace std::string_literals;

std::shared_ptr<TestStorage> parseCode(const std::string& code, const std::vector<std::wstring>& compilerFlags = {}) {
  return cxx_test::parseCode(code, L"-std=c++20", compilerFlags);
}

struct CxxParser20TestSuite : cxx_test::CxxParserStdTest {};

/**
 * https://en.cppreference.com/w/cpp/language/constraints
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0734r0.pdf
 */
TEST_F(CxxParser20TestSuite, conceptDefinition) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T>
concept Integral = __is_integral(T);)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
}

/**
 * https://en.cppreference.com/w/cpp/language/constraints#Requires_clauses
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0734r0.pdf
 */
TEST_F(CxxParser20TestSuite, requiresClause) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T>
concept Integral = __is_integral(T);

template<typename T> requires Integral<T>
T twice(T value) { return value * 2; })");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"T twice<typename T>(T) <4:1 <5:1 <5:3 5:7> 5:16> 5:38>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/requires
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0734r0.pdf
 */
TEST_F(CxxParser20TestSuite, requiresExpression) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T>
concept Addable = requires(T a, T b) { a + b; };)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
}

/**
 * Abbreviated function templates
 * https://en.cppreference.com/w/cpp/language/function_template
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p1141r2.html
 */
TEST_F(CxxParser20TestSuite, abbreviatedFunctionTemplates) {
  const std::shared_ptr<TestStorage> client = parseCode("void func(auto value) {}");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"void func<class value:auto>(value:auto) <1:1 <1:1 <1:6 1:9> 1:21> 1:24>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/auto#Constrained_auto
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p1141r2.html
 */
TEST_F(CxxParser20TestSuite, constrainedAuto) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T>
concept Integral = __is_integral(T);

Integral auto value = 10;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int value <4:15 4:19>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/operator_comparison#Three-way_comparison
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0515r3.pdf
 */
TEST_F(CxxParser20TestSuite, threeWayComparison) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Point {
  int x;
  int operator<=>(const Point& other) const { return x - other.x; }
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(
      client->methods, testing::Contains(L"public int Point::operator<=>(const Point &) const <3:3 <3:3 <3:7 3:17> 3:43> 3:67>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/aggregate_initialization#Designated_initializers
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0329r4.pdf
 */
TEST_F(CxxParser20TestSuite, designatedInitializers) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Point { int x; int y; };
Point origin { .x = 1, .y = 2 };)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"Point origin <2:7 2:12>"));
  // The designators themselves are not recorded: the indexer has no `DesignatedInitExpr` handling,
  // so `.x` / `.y` produce no reference to `Point::x` / `Point::y`.
  EXPECT_THAT(client->usages, testing::IsEmpty());
}

/**
 * https://en.cppreference.com/w/cpp/language/consteval
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1073r3.html
 */
TEST_F(CxxParser20TestSuite, constevalFunctions) {
  const std::shared_ptr<TestStorage> client = parseCode("consteval int square(int x) { return x * x; }");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int square(int) <1:1 <1:1 <1:15 1:20> 1:27> 1:45>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/constinit
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1143r2.md
 */
TEST_F(CxxParser20TestSuite, constinitVariables) {
  const std::shared_ptr<TestStorage> client = parseCode("constinit int counter = 0;");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int counter <1:15 1:21>"));
}

/**
 * constexpr virtual functions, try/catch and dynamic allocation
 * https://en.cppreference.com/w/cpp/language/constexpr
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1064r0.html
 */
TEST_F(CxxParser20TestSuite, extendedConstexpr) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Base {
  constexpr virtual int value() const { return 1; }
};
struct Derived : Base {
  constexpr int value() const override { return 2; }
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->overrides, testing::Contains(L"int Derived::value() const -> int Base::value() const <5:17 5:21>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/template_parameters#Non-type_template_parameter
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1907r1.html
 */
TEST_F(CxxParser20TestSuite, classTypesAsNonTypeTemplateParameters) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Point { int x; int y; };
template<Point P> struct Holder {};
Holder<Point{1, 2}> holder;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->structs, testing::Contains(L"Holder<Point P> <2:1 <2:26 2:31> 2:34>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/lambda#Template_parameter_list
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0428r2.pdf
 */
TEST_F(CxxParser20TestSuite, templateParameterListForLambdas) {
  const std::shared_ptr<TestStorage> client = parseCode("auto func = []<typename T>(T value) { return value; };");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"lambda at 1:13 func <1:6 1:9>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/lambda#Lambda_capture
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0806r2.html
 */
TEST_F(CxxParser20TestSuite, lambdaCaptureOfThisWithEqualsIsDeprecated) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Point {
  int x;
  int getX() { return [=, this]() { return x; }(); }
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->methods, testing::Contains(L"public int Point::getX() <3:3 <3:3 <3:7 3:10> 3:12> 3:52>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/range-for
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0614r1.html
 */
TEST_F(CxxParser20TestSuite, initStatementInRangeBasedFor) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(int sum() {
  int total = 0;
  for (int values[] = {1, 2, 3}; int value : values) {
    total += value;
  }
  return total;
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:38> <3:38 3:42>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/attributes/likely
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0479r5.html
 */
TEST_F(CxxParser20TestSuite, likelyAndUnlikelyAttributes) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(int func(bool condition) {
  if (condition) [[likely]] {
    return 1;
  }
  return 0;
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int func(bool) <1:1 <1:1 <1:5 1:8> 1:24> 6:1>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/attributes/no_unique_address
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0840r2.html
 */
TEST_F(CxxParser20TestSuite, noUniqueAddressAttribute) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Empty {};
struct Foo {
  [[no_unique_address]] Empty empty;
  int value;
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->fields, testing::Contains(L"public Empty Foo::empty <3:31 3:35>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/explicit
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0892r2.html
 */
TEST_F(CxxParser20TestSuite, conditionallyExplicitConstructors) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T>
struct Holder {
  explicit(sizeof(T) > 4) Holder(T value);
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->methods, testing::Contains(L"public void Holder<typename T>::Holder<T>(T) <3:3 <3:27 3:32> 3:41>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/types#char8_t
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0482r6.html
 */
TEST_F(CxxParser20TestSuite, char8_tType) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(const char8_t* text = u8"text";
char8_t letter = u8'x';)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"const char8_t * text <1:16 1:19>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"char8_t letter <2:9 2:14>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/enum#Using-enum-declaration
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1099r5.html
 */
TEST_F(CxxParser20TestSuite, usingEnumDeclaration) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(enum class Color { Red, Green };
int func() {
  using enum Color;
  return static_cast<int>(Red);
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->enumConstants, testing::Contains(L"Color::Red <1:20 1:22>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/namespace#Syntax
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1094r2.html
 */
TEST_F(CxxParser20TestSuite, nestedInlineNamespaces) {
  const std::shared_ptr<TestStorage> client = parseCode("namespace A::inline B { int value; }");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->namespaces, testing::Contains(L"A::B <1:14 <1:21 1:21> 1:36>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int A::B::value <1:29 1:33>"));
}

/**
 * https://en.cppreference.com/w/cpp/preprocessor/replace#Function-like_macros
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0306r4.html
 */
TEST_F(CxxParser20TestSuite, vaOptInVariadicMacros) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(#define LOG(format, ...) print(format __VA_OPT__(,) __VA_ARGS__)
void print(const char* format, ...);
void func() { LOG("text"); })");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->macros, testing::Contains(L"LOG <1:9 <1:9 1:11> 1:63>"));
  EXPECT_THAT(client->macroUses, testing::Contains(L"temp.cpp -> LOG <3:15 3:17>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/aggregate_initialization
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html
 */
TEST_F(CxxParser20TestSuite, parenthesizedInitializationOfAggregates) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Point { int x; int y; };
Point origin(1, 2);)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"Point origin <2:7 2:12>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/class_template_argument_deduction
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1814r0.html
 */
TEST_F(CxxParser20TestSuite, classTemplateArgumentDeductionForAggregatesAndAliases) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T> struct Holder { T value; };
template<typename T> using Alias = Holder<T>;
Holder aggregate{10};
Alias alias{10};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"Holder<int> aggregate <3:8 3:16>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"Holder<int> alias <4:7 4:11>"));
}

/**
 * TODO: Add a test for the following feature
 * The indexer has no `CoroutineBodyStmt` / `co_await` handling, so nothing is recorded for the
 * coroutine keywords themselves.
 *
 * https://en.cppreference.com/w/cpp/language/coroutines
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0912r5.html
 */
TEST_F(CxxParser20TestSuite, coroutines) {}

/**
 * TODO: Add a test for the following feature
 * Modules need more than one translation unit and a module cache, which the single-string
 * `parseCode` helper cannot provide; the indexer also has no `ModuleDecl` / `ImportDecl` handling.
 *
 * https://en.cppreference.com/w/cpp/language/modules
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1103r3.pdf
 */
TEST_F(CxxParser20TestSuite, modules) {}

/**
 * TODO: Add a test for the following feature
 *
 * https://en.cppreference.com/w/cpp/language/constexpr
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1330r0.pdf
 */
TEST_F(CxxParser20TestSuite, changingTheActiveMemberOfAUnionInsideConstexpr) {}

/**
 * TODO: Add a test for the following feature
 *
 * https://en.cppreference.com/w/cpp/language/adl
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0846r0.html
 */
TEST_F(CxxParser20TestSuite, adlAndFunctionTemplates) {}
}    // namespace
