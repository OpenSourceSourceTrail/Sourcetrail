/**
 * @file CxxParser17TestSuite.cpp
 * @author Ahmed Abdelaal (eng.ahmedhussein89@gmail.com)
 * @brief Test the CxxParser with C++17 features
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
 * https://en.cppreference.com/w/cpp/compiler_support/17
 * https://en.cppreference.com/w/cpp/17
 */

using namespace std::string_literals;

std::shared_ptr<TestStorage> parseCode(const std::string& code, const std::vector<std::wstring>& compilerFlags = {}) {
  auto storage = std::make_shared<IntermediateStorage>();

  CxxParser parser(std::make_shared<ParserClientImpl>(storage.get()),
                   std::make_shared<TestFileRegister>(),
                   std::make_shared<IndexerStateInfo>());

  parser.buildIndex(L"temp.cpp",
                    TextAccess::createFromString(code),
                    utility::concat(compilerFlags, std::vector<std::wstring>(1, L"-std=c++17")));

  return TestStorage::create(storage);
}

struct CxxParser17TestSuite : testing::Test {
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
 * https://en.cppreference.com/w/cpp/language/auto
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3922.html
 */
TEST_F(CxxParser17TestSuite, newAutoRulesForDirectListInitialization) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(#include <initializer_list>
auto x2 = {1, 2, 3};
auto x3 {3};
auto x4 {3.0};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"std::initializer_list<int> x2 <2:6 2:7>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int x3 <3:6 3:7>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"double x4 <4:6 4:7>"));
}

/**
 * TODO: Add a test for the following feature
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4051.html
 */
TEST_F(CxxParser17TestSuite, typenameInTemplateParameter) {}

/**
 * https://en.cppreference.com/w/cpp/language/namespace#Syntax
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4230.html
 */
TEST_F(CxxParser17TestSuite, nestedNamespaceDefinition) {
  const std::shared_ptr<TestStorage> client = parseCode("namespace A::B::C { int i; }");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->namespaces, testing::Contains(L"A::B::C <1:15 <1:17 1:17> 1:28>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int A::B::C::i <1:25 1:25>"));
}

/**
 * TODO: Add a test for the following feature
 * https://en.cppreference.com/w/cpp/language/static_assert
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3928.pdf
 */
TEST_F(CxxParser17TestSuite, static_assertWithNoMessage) {}

/**
 * https://en.cppreference.com/w/cpp/language/character_literal
 */
TEST_F(CxxParser17TestSuite, u8CharacterLiterals) {
  const std::shared_ptr<TestStorage> client = parseCode("char x = u8'x';");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"char x <1:6 1:6>"));
}

/**
 * TODO: Add a test for the following feature
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4268.html
 */
TEST_F(CxxParser17TestSuite, allowConstantEvaluationForAllNonTypeTemplateArguments) {}

/**
 * TODO: Add a test for the following feature
 * https://en.cppreference.com/w/cpp/language/fold#Explanation
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0036r0.pdf
 */
TEST_F(CxxParser17TestSuite, unaryFoldExpressionsAndEmptyParameterPacks) {}

/**
 * TODO: Add a test for the following feature
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0012r1.html
 */
TEST_F(CxxParser17TestSuite, makeExceptionSpecificationsPartOfTheTypeSystem) {}

/**
 * TODO: Add a test for the following feature
 * https://en.cppreference.com/w/cpp/preprocessor/include
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0061r1.html
 */
TEST_F(CxxParser17TestSuite, has_includeInPreprocessorConditionals) {}

/**
 * TODO: Add a test for the following feature
 * https://en.cppreference.com/w/cpp/language/using_declaration#Inheriting_constructors
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0136r1.html
 */
TEST_F(CxxParser17TestSuite, newSpecificationForInheritingConstructors) {}

/**
 * TODO: Add a test for the following feature
 * https://en.cppreference.com/w/cpp/language/aggregate_initialization
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0017r1.html
 */
TEST_F(CxxParser17TestSuite, aggregateClassesWithBaseClasses) {}

/**
 * TODO: Add a test for the following feature
 * https://en.cppreference.com/w/cpp/language/fold#Explanation
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0036r0.pdf
 */
TEST_F(CxxParser17TestSuite, foldingExpressions) {}

/**
 * https://en.cppreference.com/w/cpp/language/lambda
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0018r3.html
 */
TEST_F(CxxParser17TestSuite, lambdaCaptureOfThis) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Point {
  int x {};
  int y {};
  int getX() {
    return [*this]() { return x; }();
  }
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  // Compiler will generate the copy constructor coz lambda capture `*this` create a copy of the Point
  EXPECT_THAT(client->methods, testing::Contains(L"public void Point::Point(const Point &) <1:8 <1:8 1:12> 1:12>"));
}

/**
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0138r2.pdf
 */
TEST_F(CxxParser17TestSuite, directListInitializationOfEnumerations) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(enum byte : unsigned char {};
byte b {0};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  ASSERT_THAT(client->enums, testing::Contains(L"byte <1:1 <1:6 1:9> 1:28>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"byte b <2:6 2:6>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/lambda
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0170r1.pdf
 */
TEST_F(CxxParser17TestSuite, constexprLambdaExpressions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(auto func = [](int v) constexpr { return v; };
static_assert(func(10) == 10);)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"lambda at 1:13 func <1:6 1:9>"));
}

/**
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0127r2.html
 */
TEST_F(CxxParser17TestSuite, declaringNonTypeTemplateParametersWithAuto) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<auto n> struct B { /* ... */ };
B<5> b1;
B<'a'> b2;
)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
}

// TODO(Hussein89): Enable this test on Windows after investigating the issue
#ifndef _WIN32
/**
 * https://en.cppreference.com/w/cpp/language/structured_binding
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0217r3.html
 */
TEST_F(CxxParser17TestSuite, structuredBindings) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(#include <utility>

using Coordinate = std::pair<int, int>;
Coordinate origin() {
  return Coordinate{0, 0};
}

const auto [ x, y ] = origin();)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables,
              testing::Contains(L"const std::tuple_element<0UL, const std::pair<int, int>>::type && x <8:14 8:14>"));
  EXPECT_THAT(client->globalVariables,
              testing::Contains(L"const std::tuple_element<1UL, const std::pair<int, int>>::type && y <8:17 8:17>"));
}
#endif

/**
 * https://en.cppreference.com/w/cpp/language/if
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0292r2.html
 */
TEST_F(CxxParser17TestSuite, constexprIf) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(#include <type_traits>
template <typename T>
constexpr bool isIntegral() {
  if constexpr (std::is_integral<T>::value) {
    return true;
  } else {
    return false;
  }
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"bool isIntegral<typename T>() <2:1 <3:1 <3:16 3:25> 3:27> 9:1>"));
}

/**
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0305r1.html
 */
TEST_F(CxxParser17TestSuite, initStatementsForIfAndSwitch) {
  const std::shared_ptr<TestStorage> client = parseCode("int main() { if(bool value = true) {} }");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int main() <1:1 <1:1 <1:5 1:8> 1:10> 1:39>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/inline
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0386r2.pdf
 */
TEST_F(CxxParser17TestSuite, inlineVariables) {
  const std::shared_ptr<TestStorage> client = parseCode("inline int x = 10;");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int x <1:12 1:12>"));
}

/**
 * Class template argument deduction (CTAD)
 * https://en.cppreference.com/w/cpp/language/class_template_argument_deduction
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0091r3.html
 */
TEST_F(CxxParser17TestSuite, classTemplateArgumentDeduction) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T>
struct Type {
    Type(T v) : value{v} {}

    T value;
};

auto global = Type{10};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  const std::vector ExpectedValues = {L"Type<typename T> <1:1 <2:8 2:11> 6:1>"s, L"Type<int> <1:1 <2:8 2:11> 6:1>"s};
  ASSERT_THAT(client->structs, testing::ContainerEq(ExpectedValues));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"Type<int> global <8:6 8:11>"));
}
}    // namespace
