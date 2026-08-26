/**
 * @file CxxParser11TestSuite.cpp
 * @author Ahmed Abdelaal (eng.ahmedhussein89@gmail.com)
 * @brief Test the CxxParser with C++11 features
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
 * https://en.cppreference.com/w/cpp/compiler_support/11
 * https://en.cppreference.com/w/cpp/11
 */

using namespace std::string_literals;

std::shared_ptr<TestStorage> parseCode(const std::string& code, const std::vector<std::wstring>& compilerFlags = {}) {
  return cxx_test::parseCode(code, L"-std=c++11", compilerFlags);
}

struct CxxParser11TestSuite : cxx_test::CxxParserStdTest {};

/**
 * https://en.cppreference.com/w/cpp/language/auto
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2546.htm
 */
TEST_F(CxxParser11TestSuite, autoTypeDeduction) {
  const std::shared_ptr<TestStorage> client = parseCode("auto x = 10;");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int x <1:6 1:6>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/decltype
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2343.pdf
 */
TEST_F(CxxParser11TestSuite, decltypeSpecifier) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(int i = 0;
decltype(i) j = i;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int i <1:5 1:5>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int j <2:13 2:13>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/function#Return_type_deduction
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2445.html
 */
TEST_F(CxxParser11TestSuite, trailingReturnType) {
  const std::shared_ptr<TestStorage> client = parseCode("auto sum(int a, int b) -> int { return a + b; }");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int sum(int, int) <1:1 <1:1 <1:6 1:8> 1:29> 1:47>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/reference#Rvalue_references
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n2118.html
 */
TEST_F(CxxParser11TestSuite, rvalueReferences) {
  const std::shared_ptr<TestStorage> client = parseCode("void func(int&& value) {}");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"void func(int &&) <1:1 <1:1 <1:6 1:9> 1:22> 1:25>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/move_constructor
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2010/n3053.html
 */
TEST_F(CxxParser11TestSuite, moveConstructorAndMoveAssignment) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Foo {
  Foo(Foo&& other) noexcept;
  Foo& operator=(Foo&& other) noexcept;
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->methods, testing::Contains(L"public void Foo::Foo(Foo &&) <2:3 <2:3 2:5> 2:27>"));
  EXPECT_THAT(client->methods, testing::Contains(L"public Foo & Foo::operator=(Foo &&) <3:3 <3:8 3:16> 3:38>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/parameter_pack
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2242.pdf
 */
TEST_F(CxxParser11TestSuite, variadicTemplates) {
  const std::shared_ptr<TestStorage> client = parseCode("template<typename... Args> void func(Args... args) {}");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"void func<typename... Args>(Args) <1:1 <1:28 <1:33 1:36> 1:50> 1:53>"));
}

/**
 * https://en.cppreference.com/w/cpp/utility/initializer_list
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2672.htm
 */
TEST_F(CxxParser11TestSuite, initializerLists) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(#include <initializer_list>
std::initializer_list<int> values = {1, 2, 3};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"std::initializer_list<int> values <2:28 2:33>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/list_initialization
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2640.pdf
 */
TEST_F(CxxParser11TestSuite, uniformInitialization) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Point { int x; int y; };
Point origin {0, 0};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->structs, testing::Contains(L"Point <1:1 <1:8 1:12> 1:30>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"Point origin <2:7 2:12>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/lambda
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n2927.pdf
 */
TEST_F(CxxParser11TestSuite, lambdaExpressions) {
  const std::shared_ptr<TestStorage> client = parseCode("auto func = [](int x) { return x; };");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"lambda at 1:13 func <1:6 1:9>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/constexpr
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2235.pdf
 */
TEST_F(CxxParser11TestSuite, constexprFunctions) {
  const std::shared_ptr<TestStorage> client = parseCode("constexpr int square(int x) { return x * x; }");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"int square(int) <1:1 <1:1 <1:15 1:20> 1:27> 1:45>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/nullptr
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2431.pdf
 */
TEST_F(CxxParser11TestSuite, nullptrConstant) {
  const std::shared_ptr<TestStorage> client = parseCode("int* pointer = nullptr;");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int * pointer <1:6 1:12>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/enum#Scoped_enumerations
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2347.pdf
 */
TEST_F(CxxParser11TestSuite, scopedAndStronglyTypedEnums) {
  const std::shared_ptr<TestStorage> client = parseCode("enum class Color : int { Red, Green };");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->enums, testing::Contains(L"Color <1:1 <1:12 1:16> 1:37>"));
  EXPECT_THAT(client->enumConstants, testing::Contains(L"Color::Red <1:26 1:28>"));
  EXPECT_THAT(client->enumConstants, testing::Contains(L"Color::Green <1:31 1:35>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/enum
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2764.pdf
 */
TEST_F(CxxParser11TestSuite, forwardDeclaredEnums) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(enum class Color : int;
enum class Color : int { Red };)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->enums, testing::Contains(L"Color <2:1 <2:12 2:16> 2:30>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/static_assert
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2004/n1720.html
 */
TEST_F(CxxParser11TestSuite, staticAssertions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(static_assert(sizeof(int) >= 4, "int is too small");
int value = 0;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int value <2:5 2:9>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/template_parameters
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1757.html
 */
TEST_F(CxxParser11TestSuite, rightAngleBrackets) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T> struct Holder { T value; };
Holder<Holder<int>> nested;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"Holder<Holder<int>> nested <2:21 2:26>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/type_alias
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2258.pdf
 */
TEST_F(CxxParser11TestSuite, aliasTemplates) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(using Integer = int;
template<typename T> using Pointer = T*;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->typedefs, testing::Contains(L"Integer <1:7 1:13>"));
  EXPECT_THAT(client->typedefs, testing::Contains(L"Pointer<typename T> <2:28 2:34>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/class_template#Explicit_instantiation
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n1987.htm
 */
TEST_F(CxxParser11TestSuite, externTemplates) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T> struct Holder { T value; };
extern template struct Holder<int>;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->structs, testing::Contains(L"Holder<typename T> <1:1 <1:29 1:34> 1:47>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/constructor#Delegating_constructor
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n1986.pdf
 */
TEST_F(CxxParser11TestSuite, delegatingConstructors) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Foo {
  Foo(int v) : value(v) {}
  Foo() : Foo(0) {}
  int value;
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->methods, testing::Contains(L"public void Foo::Foo(int) <2:3 <2:3 <2:3 2:5> 2:12> 2:26>"));
  EXPECT_THAT(client->calls, testing::Contains(L"void Foo::Foo() -> void Foo::Foo(int) <3:11 3:13>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/using_declaration#Inheriting_constructors
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2540.htm
 */
TEST_F(CxxParser11TestSuite, inheritingConstructors) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Base { Base(int v); };
struct Derived : Base { using Base::Base; };)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->inheritances, testing::Contains(L"Derived -> Base <2:18 2:21>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/function#Deleted_functions
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2346.htm
 */
TEST_F(CxxParser11TestSuite, defaultedAndDeletedFunctions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Foo {
  Foo() = default;
  Foo(const Foo&) = delete;
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->methods, testing::Contains(L"public void Foo::Foo() <2:3 <2:3 2:5> 2:17>"));
  EXPECT_THAT(client->methods, testing::Contains(L"public void Foo::Foo(const Foo &) <3:3 <3:3 3:5> 3:26>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/override
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2010/n3163.pdf
 */
TEST_F(CxxParser11TestSuite, overrideAndFinal) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Base { virtual void func(); };
struct Derived final : Base { void func() override; };)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->overrides, testing::Contains(L"void Derived::func() -> void Base::func() <2:36 2:39>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/data_members#Member_initialization
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2756.htm
 */
TEST_F(CxxParser11TestSuite, nonStaticDataMemberInitializers) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Foo {
  int value = 10;
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->fields, testing::Contains(L"public int Foo::value <2:7 2:11>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/union#Unrestricted_unions
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2544.pdf
 */
TEST_F(CxxParser11TestSuite, unrestrictedUnions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Point { Point() {} int x; };
union Value {
  int number;
  Point point;
  Value() {}
  ~Value() {}
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->unions, testing::Contains(L"Value <2:1 <2:7 2:11> 7:1>"));
  EXPECT_THAT(client->fields, testing::Contains(L"public Point Value::point <4:9 4:13>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/union#Anonymous_unions
 */
TEST_F(CxxParser11TestSuite, anonymousStructsAndUnions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(struct Foo {
  union {
    int number;
    float decimal;
  };
};)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->fields, testing::Contains(L"public int Foo::anonymous union (temp.cpp<2:3>)::number <3:9 3:14>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/string_literal
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2442.htm
 */
TEST_F(CxxParser11TestSuite, rawStringLiterals) {
  const std::shared_ptr<TestStorage> client = parseCode("const char* text = R\"(raw \"string\" literal)\";");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"const char * text <1:13 1:16>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/user_literal
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2765.pdf
 */
TEST_F(CxxParser11TestSuite, userDefinedLiterals) {
  const std::shared_ptr<TestStorage> client = parseCode(
      R"(constexpr long double operator"" _km(long double value) { return value * 1000.0; }
long double distance = 3.0_km;)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"long double distance <2:13 2:20>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/types#Character_types
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2249.html
 */
TEST_F(CxxParser11TestSuite, unicodeCharacterTypesAndLiterals) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(const char* utf8 = u8"text";
char16_t utf16 = u'x';
char32_t utf32 = U'x';)");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"const char * utf8 <1:13 1:16>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"char16_t utf16 <2:10 2:14>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"char32_t utf32 <3:10 3:14>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/attributes
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2761.pdf
 */
TEST_F(CxxParser11TestSuite, generalizedAttributes) {
  const std::shared_ptr<TestStorage> client = parseCode("[[noreturn]] void fail();");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"void fail() <1:14 <1:19 1:22> 1:24>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/noexcept_spec
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n3050.html
 */
TEST_F(CxxParser11TestSuite, noexceptSpecifier) {
  const std::shared_ptr<TestStorage> client = parseCode("void func() noexcept {}");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"void func() <1:1 <1:1 <1:6 1:9> 1:20> 1:23>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/range-for
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n2930.html
 */
TEST_F(CxxParser11TestSuite, rangeBasedForLoop) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(int sum(const int (&values)[3]) {
  int total = 0;
  for (int value : values) {
    total += value;
  }
  return total;
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:12> <3:12 3:16>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/storage_duration#Thread_local_storage
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2659.htm
 */
TEST_F(CxxParser11TestSuite, threadLocalStorage) {
  const std::shared_ptr<TestStorage> client = parseCode("thread_local int counter = 0;");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int counter <1:18 1:24>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/namespace#Inline_namespaces
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2535.htm
 */
TEST_F(CxxParser11TestSuite, inlineNamespaces) {
  const std::shared_ptr<TestStorage> client = parseCode("inline namespace v1 { int value; }");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->namespaces, testing::Contains(L"v1 <1:1 <1:18 1:19> 1:34>"));
  EXPECT_THAT(client->globalVariables, testing::Contains(L"int v1::value <1:27 1:31>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/types#Standard_integer_types
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1811.pdf
 */
TEST_F(CxxParser11TestSuite, longLongInteger) {
  const std::shared_ptr<TestStorage> client = parseCode("long long big = 9223372036854775807LL;");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->globalVariables, testing::Contains(L"long long big <1:11 1:13>"));
}

/**
 * https://en.cppreference.com/w/cpp/language/sfinae
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2634.html
 */
TEST_F(CxxParser11TestSuite, sfinaeOnExpressions) {
  const std::shared_ptr<TestStorage> client = parseCode(R"(template<typename T>
auto size(const T& value) -> decltype(value.size()) {
  return value.size();
})");

  ASSERT_THAT(client->errors, testing::IsEmpty());
  EXPECT_THAT(client->functions, testing::Contains(L"<dependent type> size<typename T>(const T &) <1:1 <2:1 <2:6 2:9> 2:51> 4:1>"));
}
}    // namespace
