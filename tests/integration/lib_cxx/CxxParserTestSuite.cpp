#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../../src/lib/tests/mocks/MockedApplicationSetting.hpp"
#include "CxxParser.h"
#include "IndexerCommandCxx.h"
#include "IndexerStateInfo.h"
#include "ParserClientImpl.h"
#include "TestFileRegister.h"
#include "TestStorage.h"
#include "TextAccess.h"
#include "gmock/gmock.h"
#include "language_packages.h"
#include "utility.h"
#include "utilityString.h"

namespace {

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

}    // namespace

struct CxxParserTestSuite : testing::Test {
  void SetUp() override {
    IApplicationSettings::setInstance(mMocked);
    EXPECT_CALL(*mMocked, getLoggingEnabled).WillRepeatedly(testing::Return(false));
  }
  void TearDown() override {
    IApplicationSettings::setInstance(nullptr);
    mMocked.reset();
  }
  std::shared_ptr<MockedApplicationSettings> mMocked = std::make_shared<MockedApplicationSettings>();
};

TEST_F(CxxParserTestSuite, cxxParserFindsGlobalVariableDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("int x;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->globalVariables, L"int x <1:5 1:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStaticGlobalVariableDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("static int x;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->globalVariables, L"int x (temp.cpp) <1:12 1:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStaticConstGlobalVariableDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("static const int x;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->globalVariables, L"const int x (temp.cpp) <1:18 1:18>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsGlobalClassDefinition) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"A <1:1 <1:7 1:7> 3:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsGlobalClassDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("class A;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"A <1:7 1:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsGlobalStructDefinition) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"A <1:1 <1:8 1:8> 3:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsGlobalStructDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("struct A;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"A <1:8 1:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsVariableDefinitionsInGlobalScope) {
  std::shared_ptr<TestStorage> client = parseCode("int x;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->globalVariables, L"int x <1:5 1:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsFieldsInClassWithAccessType) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	int a;\n"
      "public:\n"
      "	A() : d(0) {};\n"
      "	int b;\n"
      "protected:\n"
      "	static int c;\n"
      "private:\n"
      "	const int d;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->fields, L"private int A::a <3:6 3:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->fields, L"public int A::b <6:6 6:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->fields, L"protected static int A::c <8:13 8:13>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->fields, L"private const int A::d <10:12 10:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsFunctionDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int ceil(float a)\n"
      "{\n"
      "	return 1;\n"
      "}\n");

  EXPECT_THAT(client->functions, testing::Contains(testing::StrEq(L"int ceil(float) <1:1 <1:1 <1:5 1:8> 1:17> 4:1>")));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStaticFunctionDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "static int ceil(float a)\n"
      "{\n"
      "	return static_cast<int>(a) + 1;\n"
      "}\n");

  EXPECT_THAT(client->functions,
              testing::Contains(testing::StrEq(L"static int ceil(float) (temp.cpp) <1:1 <1:1 <1:12 1:15> 1:24> 4:1>")));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMethodDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	B();\n"
      "};\n");

  EXPECT_THAT(client->methods, testing::Contains(testing::StrEq(L"public void B::B() <4:2 <4:2 4:2> 4:4>")));
}

TEST_F(CxxParserTestSuite, cxxParserFindsOverloadedOperatorDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	B& operator=(const B& other);\n"
      "};\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->methods, L"public B & B::operator=(const B &) <4:2 <4:5 4:13> 4:29>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMethodDeclarationAndDefinition) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	B();\n"
      "};\n"
      "B::B()\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->methods, L"public void B::B() <6:1 <6:4 6:4> 8:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsVirtualMethodDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	virtual void process();\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->methods, L"public void B::process() <4:2 <4:15 4:21> 4:23>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsPureVirtualMethodDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "protected:\n"
      "	virtual void process() = 0;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->methods, L"protected void B::process() <4:2 <4:15 4:21> 4:27>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNamedNamespaceDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace A\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->namespaces, L"A <1:1 <1:11 1:11> 3:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsAnonymousNamespaceDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->namespaces, L"anonymous namespace (temp.cpp<1:1>) <1:1 <2:1 2:1> 3:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMultipleAnonymousNamespaceDeclarationsAsSameSymbol) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "}\n"
      "namespace\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->namespaces, L"anonymous namespace (temp.cpp<1:1>) <1:1 <2:1 2:1> 3:1>"));

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->namespaces, L"anonymous namespace (temp.cpp<1:1>) <4:1 <5:1 5:1> 6:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMultipleNestedAnonymousNamespaceDeclarationsAsDifferentSymbol) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "	namespace\n"
      "	{\n"
      "	}\n"
      "}\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->namespaces, L"anonymous namespace (temp.cpp<1:1>) <1:1 <2:1 2:1> 6:1>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->namespaces,
      L"anonymous namespace (temp.cpp<1:1>)::anonymous namespace (temp.cpp<3:2>) <3:2 <4:2 4:2> "
      L"5:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsAnonymousNamespaceDeclarationsNestedInsideNamespacesWithDifferentNameAsDifferentSymbol) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace a\n"
      "{\n"
      "	namespace\n"
      "	{\n"
      "	}\n"
      "}\n"
      "namespace b\n"
      "{\n"
      "	namespace\n"
      "	{\n"
      "	}\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->namespaces, L"a <1:1 <1:11 1:11> 6:1>"));

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->namespaces, L"a::anonymous namespace (temp.cpp<3:2>) <3:2 <4:2 4:2> 5:2>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->namespaces, L"b <7:1 <7:11 7:11> 12:1>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->namespaces, L"b::anonymous namespace (temp.cpp<9:2>) <9:2 <10:2 10:2> 11:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsAnonymousNamespaceDeclarationsNestedInsideNamespacesWithSameNameAsSameSymbol) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace a\n"
      "{\n"
      "	namespace\n"
      "	{\n"
      "	}\n"
      "}\n"
      "namespace a\n"
      "{\n"
      "	namespace\n"
      "	{\n"
      "	}\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->namespaces, L"a <1:1 <1:11 1:11> 6:1>"));

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->namespaces, L"a::anonymous namespace (temp.cpp<3:2>) <3:2 <4:2 4:2> 5:2>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->namespaces, L"a <7:1 <7:11 7:11> 12:1>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->namespaces, L"a::anonymous namespace (temp.cpp<3:2>) <9:2 <10:2 10:2> 11:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsAnonymousStructDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef struct\n"
      "{\n"
      "	int x;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"anonymous struct (temp.cpp<1:9>) <1:9 <1:9 1:14> 4:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMultipleAnonymousStructDeclarationsAsDistinctElements) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef struct\n"
      "{\n"
      "	int x;\n"
      "};\n"
      "typedef struct\n"
      "{\n"
      "	float x;\n"
      "};\n");

  EXPECT_TRUE(client->structs.size() == 2);
  EXPECT_TRUE(client->fields.size() == 2);
  EXPECT_TRUE(utility::substrBeforeLast(client->fields[0], '<') != utility::substrBeforeLast(client->fields[1], '<'));
}

TEST_F(CxxParserTestSuite, cxxParserFindsAnonymousUnionDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef union\n"
      "{\n"
      "	int i;\n"
      "	float f;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->unions, L"anonymous union (temp.cpp<1:9>) <1:9 <1:9 1:13> 5:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNameOfAnonymousStructDeclaredInsideTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef struct\n"
      "{\n"
      "	int x;\n"
      "} Foo;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"Foo <1:9 <1:9 1:14> 4:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"Foo <4:3 4:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNameOfAnonymousClassDeclaredInsideTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef class\n"
      "{\n"
      "	int x;\n"
      "} Foo;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"Foo <1:9 <1:9 1:13> 4:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"Foo <4:3 4:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNameOfAnonymousEnumDeclaredInsideTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef enum\n"
      "{\n"
      "	CONSTANT_1;\n"
      "} Foo;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enums, L"Foo <1:9 <1:9 1:12> 4:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enums, L"Foo <4:3 4:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNameOfAnonymousUnionDeclaredInsideTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef union\n"
      "{\n"
      "	int x;\n"
      "	float y;\n"
      "} Foo;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->unions, L"Foo <1:9 <1:9 1:13> 5:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->unions, L"Foo <5:3 5:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNameOfAnonymousStructDeclaredInsideTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "using Foo = struct\n"
      "{\n"
      "	int x;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"Foo <1:13 <1:13 1:18> 4:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"Foo <1:7 1:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNameOfAnonymousClassDeclaredInsideTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "using Foo = class\n"
      "{\n"
      "	int x;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"Foo <1:13 <1:13 1:17> 4:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"Foo <1:7 1:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNameOfAnonymousEnumDeclaredInsideTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "using Foo = enum\n"
      "{\n"
      "	CONSTANT_1;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enums, L"Foo <1:13 <1:13 1:16> 4:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enums, L"Foo <1:7 1:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNameOfAnonymousUnionDeclaredInsideTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "using Foo = union\n"
      "{\n"
      "	int x;\n"
      "	float y;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->unions, L"Foo <1:13 <1:13 1:17> 5:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->unions, L"Foo <1:7 1:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsEnumDefinedInGlobalNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum E\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enums, L"E <1:1 <1:6 1:6> 3:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsEnumConstantInGlobalEnum) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum E\n"
      "{\n"
      "	P\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enumConstants, L"E::P <3:2 3:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypedefInGlobalNamespace) {
  std::shared_ptr<TestStorage> client = parseCode("typedef unsigned int uint;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typedefs, L"uint <1:22 1:25>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypedefInNamedNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace test\n"
      "{\n"
      "	typedef unsigned int uint;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typedefs, L"test::uint <3:23 3:26>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypedefInAnonymousNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "	typedef unsigned int uint;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typedefs, L"anonymous namespace (temp.cpp<1:1>)::uint <3:23 3:26>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeAliasInClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Foo\n"
      "{\n"
      "	using Bar = Foo;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typedefs, L"private Foo::Bar <3:8 3:10>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroDefine) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI\n"
      "void test()\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->macros, L"PI <1:9 1:10>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroUndefine) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#undef PI\n"
      "void test()\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->macroUses, L"temp.cpp -> PI <1:8 1:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroInIfdef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI\n"
      "#ifdef PI\n"
      "void test()\n"
      "{\n"
      "};\n"
      "#endif\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->macroUses, L"temp.cpp -> PI <2:8 2:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroInIfndef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI\n"
      "#ifndef PI\n"
      "void test()\n"
      "{\n"
      "};\n"
      "#endif\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->macroUses, L"temp.cpp -> PI <2:9 2:10>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroInIfdefined) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI\n"
      "#if defined(PI)\n"
      "void test()\n"
      "{\n"
      "};\n"
      "#endif\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->macroUses, L"temp.cpp -> PI <2:13 2:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroExpand) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI 3.14159265359\n"
      "void test()\n"
      "{\n"
      "double i = PI;"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->macroUses, L"temp.cpp -> PI <4:12 4:13>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroExpandWithinMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI 3.14159265359\n"
      "#define TAU (2 * PI)\n"
      "void test()\n"
      "{\n"
      "double i = TAU;"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->macroUses, L"temp.cpp -> PI <2:18 2:19>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroDefineScope) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define MAX(a,b) \\\n"
      "	((a)>(b)?(a):(b))");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->macros, L"MAX <1:9 <1:9 1:11> 2:17>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterDefinitionOfTemplateTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template<class T>\n"
      "using MyType = int;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:16> <1:16 1:16>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterDefinitionOfClassTemplate) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <1:20 1:20>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterDefinitionOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename U>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <typename T>\n"
      "class A<T, int>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<5:20> <5:20 5:20>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<5:20> <6:9 6:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterDefinitionOfVariableTemplate) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T v;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <1:20 1:20>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterDefinitionOfExplicitPartialVariableTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename Q>\n"
      "T t = Q(5);\n"
      "\n"
      "template <typename R>\n"
      "int t<int, R> = 9;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:20> <4:20 4:20>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:20> <5:12 5:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterDefinedWithClassKeyword) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <class T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:17> <1:17 1:17>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeIntTemplateParameterDefinitionOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:15> <1:15 1:15>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeBoolTemplateParameterDefinitionOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:16> <1:16 1:16>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeCustomPointerTemplateParameterDefinitionOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P* p>\n"
      "class A\n"
      "{};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:14> <3:14 3:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeCustomReferenceTemplateParameterDefinitionOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P& p>\n"
      "class A\n"
      "{};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:14> <3:14 3:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeTemplateParameterDefinitionThatDependsOnTypeTemplateParameterOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T1, T1& T2>\n"
      "class A\n"
      "{};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:28> <1:28 1:29>"));

  // and usage
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <1:24 1:25>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeTemplateParameterDefinitionThatDependsOnTemplateTemplateParameterOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<typename> class T1, T1<int>& T2>\n"
      "class A\n"
      "{};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:49> <1:49 1:50>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:36> <1:40 1:41>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsNonTypeTemplateParameterDefinitionThatDependsOnTypeTemplateParameterOfTemplateTemplateParameter) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<typename T, T R>typename S>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:45> <1:45 1:45>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:29> <1:32 1:32>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateArgumentOfDependentNonTypeTemplateParameter) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<typename> class T1, T1<int>& T2>\n"
      "class A\n"
      "{};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->typeUses, L"A<template<typename> typename T1, T1<int> & T2> -> int <1:43 1:45>"));
}

// void _test_foofoo()
//{
//	std::shared_ptr<TestStorage> client = parseCode(
//		"template <typename T1, typename T2>\n"
//		"class vector { };\n"
//		"\n"
//		"template<class T>\n"
//		"struct Alloc { };\n"
//		"\n"
//		"template<class T>\n"
//		"using Vec = vector<T, Alloc<T>>;\n"
//		"\n"
//		"Vec<int> v;\n"
//	);

//	TS_ASSERT(utility::containsElement<std::wstring>(
//		client->typeUses, // TODO: record edge between vector<int, Alloc<int>> and Alloc<int> (this
// is an issue because we don't have any typeloc for this edge -.-
//	));
//}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateTemplateParameterOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T>\n"
      "class B\n"
      "{};\n"
      "int main()\n"
      "{\n"
      "	B<A> ba;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:36> <4:36 4:36>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterPackTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename... T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:23> <1:23 1:23>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeIntTemplateParameterPackTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int... T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:18> <1:18 1:18>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateTemplateParameterPackTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<typename> typename... T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:42> <1:42 1:42>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParametersOfTemplateClassWithMultipleParameters) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename U>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <1:20 1:20>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:32> <1:32 1:32>"));
}

TEST_F(CxxParserTestSuite, cxxParserSkipsCreatingNodeForTemplateParameterWithoutAName) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename>\n"
      "class A\n"
      "{\n"     // local symbol for brace
      "};\n"    // local symbol for brace
  );

  EXPECT_TRUE(client->localSymbols.size() == 2);
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"A<typename> <1:1 <2:7 2:7> 4:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterOfTemplateMethodDefinitionOutsideTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	template <typename U>\n"
      "	U foo();\n"
      "};\n"
      "template <typename T>\n"
      "template <typename U>\n"
      "U A<T>::foo()\n"
      "{}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<8:20> <8:20 8:20>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <>\n"
      "class A<int>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"A<int> <5:1 <6:7 6:7> 8:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitVariableTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T t = T(5);\n"
      "\n"
      "template <>\n"
      "int t<int> = 99;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->globalVariables, L"int t<int> <5:5 5:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename U>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <typename T>\n"
      "class A<T, int>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"A<typename T, int> <5:1 <6:7 6:7> 8:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitPartialVariableTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename Q>\n"
      "T t = Q(5);\n"
      "\n"
      "template <typename R>\n"
      "int t<int, R> = 9;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->globalVariables, L"int t<int, typename R> <5:5 5:5>"));
}

TEST_F(CxxParserTestSuite, CxxParserFindsCorrectFieldMemberNameOfTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	int foo;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->fields, L"private int A<typename T>::foo <4:6 4:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCorrectTypeOfFieldMemberOfTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T foo;\n"
      "};\n"
      "A<int> a; \n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int A<int>::foo -> int <4:2 4:2>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> a -> A<int> <6:1 6:1>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> a -> int <6:3 6:5>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <4:2 4:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCorrectMethodMemberNameOfTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	int foo();\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->methods, L"private int A<typename T>::foo() <4:2 <4:6 4:8> 4:10>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(T a)\n"
      "{\n"
      "	return a;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <1:20 1:20>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeIntTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"
      "int test(int a)\n"
      "{\n"
      "	return a + T;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:15> <1:15 1:15>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeBoolTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T>\n"
      "int test(int a)\n"
      "{\n"
      "	return T ? a : 0;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:16> <1:16 1:16>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeCustomPointerTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P* p>\n"
      "int test(int a)\n"
      "{\n"
      "	return a;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:14> <3:14 3:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeCustomReferenceTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P& p>\n"
      "int test(int a)\n"
      "{\n"
      "	return a;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:14> <3:14 3:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T>\n"
      "int test(int a)\n"
      "{\n"
      "	return a;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:36> <4:36 4:36>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsFunctionForImplicitInstantiationOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(T a)\n"
      "{\n"
      "	return a;\n"
      "};\n"
      "\n"
      "int main()\n"
      "{\n"
      "	return test(1);\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->functions, L"int test<int>(int) <2:1 <2:1 <2:3 2:6> 2:11> 5:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserSkipsImplicitTemplateMethodDefinitionOfImplicitTemplateClassInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	template <typename U>\n"
      "	void foo() {}\n"
      "};\n"
      "\n"
      "int main()\n"
      "{\n"
      "	A<int>().foo<float>();\n"
      "	return 0;\n"
      "}\n");

  EXPECT_TRUE(/*NOT!*/ !utility::containsElement<std::wstring>(
      client->methods, L"public void A<int>::foo<typename U>() <6:2 <6:7 6:9> 6:14>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->templateSpecializations, L"void A<int>::foo<float>() -> void A<typename T>::foo<typename U>() <6:7 6:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsLambdaDefinitionAndCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaCaller()\n"
      "{\n"
      "	[](){}();\n"
      "}\n");

  // TODO: fix
  // TS_ASSERT(utility::containsElement<std::wstring>(
  // 	client->functions, L"void lambdaCaller::lambda at 3:2() const <3:5 <3:2 3:2> 3:7>"
  // ));
  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->calls, L"void lambdaCaller() -> void lambdaCaller::lambda at 3:2() const <3:8 3:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMutableLambdaDefinition) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaWrapper()\n"
      "{\n"
      "	[](int foo) mutable { return foo; };\n"
      "}\n");

  // TODO: fix
  // TS_ASSERT(utility::containsElement<std::wstring>(
  // 	client->functions, L"int lambdaWrapper::lambda at 3:2(int) <3:14 <3:2 3:2> 3:36>"
  // ));
}

TEST_F(CxxParserTestSuite, cxxParserFindsLocalVariableDeclaredInLambdaCapture) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaWrapper()\n"
      "{\n"
      "	[x(42)]() { return x; };\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:3> <3:3 3:3>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:3> <3:21 3:21>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsDefinitionOfLocalSymbolInFunctionParameterList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void test(int a)\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:15> <1:15 1:15>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsDefinitionOfLocalSymbolInFunctionScope) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void test()\n"
      "{\n"
      "	int a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:6> <3:6 3:6>"));
}

///////////////////////////////////////////////////////////////////////////////
// test finding nested symbol definitions and declarations

TEST_F(CxxParserTestSuite, cxxParserFindsClassDefinitionInClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "public:\n"
      "	class B;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"public A::B <4:8 4:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsClassDefinitionInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace a\n"
      "{\n"
      "	class B;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"a::B <3:8 3:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStructDefinitionInClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	struct B\n"
      "	{\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"private A::B <3:2 <3:9 3:9> 5:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStructDefinitionInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace A\n"
      "{\n"
      "	struct B\n"
      "	{\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"A::B <3:2 <3:9 3:9> 5:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStructDefinitionInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void foo(int)\n"
      "{\n"
      "	struct B\n"
      "	{\n"
      "	};\n"
      "};\n"
      "void foo(float)\n"
      "{\n"
      "	struct B\n"
      "	{\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"foo::B <3:2 <3:9 3:9> 5:2>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->structs, L"foo::B <9:2 <9:9 9:9> 11:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsVariableDefinitionsInNamespaceScope) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace n"
      "{\n"
      "	int x;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->globalVariables, L"int n::x <2:6 2:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsFieldInNestedClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	class C\n"
      "	{\n"
      "	private:\n"
      "		static const int amount;\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->fields, L"private static const int B::C::amount <7:20 7:25>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsFunctionInAnonymousNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "	int sum(int a, int b);\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->functions, L"int anonymous namespace (temp.cpp<1:1>)::sum(int, int) <3:2 <3:6 3:8> 3:22>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMethodDeclaredInNestedClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "	class C\n"
      "	{\n"
      "		bool isGreat() const;\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->methods, L"private bool B::C::isGreat() const <5:3 <5:8 5:14> 5:22>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNestedNamedNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace A\n"
      "{\n"
      "	namespace B\n"
      "	{\n"
      "	}\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->namespaces, L"A::B <3:2 <3:12 3:12> 5:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsEnumDefinedInClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	enum Z\n"
      "	{\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enums, L"public B::Z <4:2 <4:7 4:7> 6:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsEnumDefinedInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace n\n"
      "{\n"
      "	enum Z\n"
      "	{\n"
      "	};\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enums, L"n::Z <3:2 <3:7 3:7> 5:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsEnumDefinitionInTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	enum TestType\n"
      "	{\n"
      "		TEST_ONE,\n"
      "		TEST_TWO\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enums, L"private A<typename T>::TestType <4:2 <4:7 4:14> 8:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsEnumConstantsInTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	enum TestType\n"
      "	{\n"
      "		TEST_ONE,\n"
      "		TEST_TWO\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->enumConstants, L"A<typename T>::TestType::TEST_ONE <6:3 6:10>"));
}

///////////////////////////////////////////////////////////////////////////////
// test qualifier locations

TEST_F(CxxParserTestSuite, cxxParserFindsQualifierOfAccessToGlobalVariableDefinedInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace foo {\n"
      "	namespace bar {\n"
      "		int x;\n"
      "	}\n"
      "}\n"
      "void f() {\n"
      "	foo::bar::x = 9;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->qualifiers, L"foo <7:2 7:4>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->qualifiers, L"foo::bar <7:7 7:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsQualifierOfAccessToStaticField) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Foo {\n"
      "public:\n"
      "	struct Bar {\n"
      "	public:\n"
      "		static int x;\n"
      "	};\n"
      "};\n"
      "void f() {\n"
      "	Foo::Bar::x = 9;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->qualifiers, L"Foo <9:2 9:4>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->qualifiers, L"Foo::Bar <9:7 9:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsQualifierOfAccessToEnumConstant) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum Foo {\n"
      "	FOO_V\n"
      "};\n"
      "void f() {\n"
      "	Foo v = Foo::FOO_V;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->qualifiers, L"Foo <5:10 5:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsQualifierOfReferenceToMethod) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Foo {\n"
      "public:\n"
      "	static void my_int_func(int x) {\n"
      "	}\n"
      "};\n"
      "\n"
      "void test() {\n"
      "	void(*foo)(int);\n"
      "	foo = &Foo::my_int_func;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->qualifiers, L"Foo <9:9 9:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsQualifierOfConstructorCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Foo {\n"
      "public:\n"
      "	Foo(int i) {}\n"
      "};\n"
      "\n"
      "class Bar : public Foo {\n"
      "public:\n"
      "	Bar() : Foo::Foo(4) {}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->qualifiers, L"Foo <8:10 8:12>"));
}

///////////////////////////////////////////////////////////////////////////////
// test implicit symbols

TEST_F(CxxParserTestSuite, cxxParserFindsBuiltinTypes) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void t1(int v) {}\n"
      "void t2(float v) {}\n"
      "void t3(double v) {}\n"
      "void t4(bool v) {}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->builtinTypes, L"void"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->builtinTypes, L"int"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->builtinTypes, L"float"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->builtinTypes, L"double"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->builtinTypes, L"bool"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsImplicitCopyConstructor) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class TestClass {}\n"
      "void foo()\n"
      "{\n"
      "	TestClass a;\n"
      "	TestClass b(a);\n"
      "}\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->methods, L"public void TestClass::TestClass() <1:7 <1:7 1:15> 1:15>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->methods, L"public void TestClass::TestClass(const TestClass &) <1:7 <1:7 1:15> 1:15>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->methods, L"public void TestClass::TestClass(TestClass &&) <1:7 <1:7 1:15> 1:15>"));
}

///////////////////////////////////////////////////////////////////////////////
// test finding usages of symbols

TEST_F(CxxParserTestSuite, cxxParserFindsEnumUsageInTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	enum TestType\n"
      "	{\n"
      "		TEST_ONE,\n"
      "		TEST_TWO\n"
      "	};\n"
      "	TestType foo;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->typeUses, L"A<typename T>::TestType A<typename T>::foo -> A<typename T>::TestType <9:2 9:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCorrectFieldMemberTypeOfNestedTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	class B\n"
      "	{\n"
      "		T foo;\n"
      "	};\n"
      "};\n"
      "A<int> a;\n"
      "A<int>::B b;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <7:3 7:3>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int A<int>::B::foo -> int <7:3 7:3>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> a -> A<int> <10:1 10:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> -> int <10:3 10:5>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> a -> int <10:3 10:5>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int>::B b -> A<int>::B <11:9 11:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeUsageOfGlobalVariable) {
  std::shared_ptr<TestStorage> client = parseCode("int x;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int x -> int <1:1 1:3>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypedefsTypeUse) {
  std::shared_ptr<TestStorage> client = parseCode("typedef unsigned int uint;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"uint -> unsigned int <1:9 1:16>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypedefThatUsesTypeDefinedInNamedNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace test\n"
      "{\n"
      "	struct TestStruct{};\n"
      "}\n"
      "typedef test::TestStruct globalTestStruct;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"globalTestStruct -> test::TestStruct <5:15 5:24>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeUseOfTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef unsigned int uint;\n"
      "uint number;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"uint number -> uint <2:1 2:4>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsClassDefaultPrivateInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B : A {};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A <2:11 2:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsClassPublicInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B : public A {};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A <2:18 2:18>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsClassProtectedInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B : protected A {};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A <2:21 2:21>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsClassPrivateInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B : private A {};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A <2:19 2:19>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsClassMultipleInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B {};\n"
      "class C\n"
      "	: public A\n"
      "	, private B\n"
      "{};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"C -> A <4:11 4:11>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"C -> B <5:12 5:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStructDefaultPublicInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B : A {};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A <2:12 2:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStructPublicInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B : public A {};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A <2:19 2:19>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStructProtectedInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B : protected A {};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A <2:22 2:22>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStructPrivateInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B : private A {};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A <2:20 2:20>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsStructMultipleInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B {};\n"
      "struct C\n"
      "	: public A\n"
      "	, private B\n"
      "{};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"C -> A <4:11 4:11>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"C -> B <5:12 5:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMethodOverrideWhenVirtual) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	virtual void foo();\n"
      "};\n"
      "class B : public A {\n"
      "	void foo();\n"
      "};");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->overrides, L"void B::foo() -> void A::foo() <5:7 5:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMultiLayerMethodOverrides) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	virtual void foo();\n"
      "};\n"
      "class B : public A {\n"
      "	void foo();\n"
      "};\n"
      "class C : public B {\n"
      "	void foo();\n"
      "};");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->overrides, L"void B::foo() -> void A::foo() <5:7 5:9>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->overrides, L"void C::foo() -> void B::foo() <8:7 8:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMethodOverridesOnDifferentReturnTypes) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	virtual void foo();\n"
      "};\n"
      "class B : public A {\n"
      "	int foo();\n"
      "};\n");

  EXPECT_TRUE(client->errors.size() == 1);
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->overrides, L"int B::foo() -> void A::foo() <5:6 5:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoMethodOverrideWhenNotVirtual) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	void foo();\n"
      "};\n"
      "class B : public A {\n"
      "	void foo();\n"
      "};");

  EXPECT_TRUE(client->overrides.size() == 0);
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoMethodOverridesOnDifferentSignatures) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	virtual void foo(int a);\n"
      "};\n"
      "class B : public A {\n"
      "	int foo(int a, int b);\n"
      "};\n");

  EXPECT_TRUE(client->overrides.size() == 0);
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsingDirectiveDeclInFunctionContext) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void foo()\n"
      "{\n"
      "	using namespace std;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void foo() -> std <3:18 3:20>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsingDirectiveDeclInFileContext) {
  std::shared_ptr<TestStorage> client = parseCode("using namespace std;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"temp.cpp -> std <1:17 1:19>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsingDeclInFunctionContext) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace foo\n"
      "{\n"
      "	int a;\n"
      "}\n"
      "void bar()\n"
      "{\n"
      "	using foo::a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void bar() -> foo::a <7:13 7:13>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsingDeclInFileContext) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace foo\n"
      "{\n"
      "	int a;\n"
      "}\n"
      "using foo::a;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"temp.cpp -> foo::a <5:12 5:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int sum(int a, int b)\n"
      "{\n"
      "	return a + b;\n"
      "}\n"
      "int main()\n"
      "{\n"
      "	sum(1, 2);\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> int sum(int, int) <7:2 7:4>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCallInFunctionWithCorrectSignature) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int sum(int a, int b)\n"
      "{\n"
      "	return a + b;\n"
      "}\n"
      "void func()\n"
      "{\n"
      "}\n"
      "void func(bool right)\n"
      "{\n"
      "	sum(1, 2);\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"void func(bool) -> int sum(int, int) <10:2 10:4>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCallToFunctionWithRightSignature) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int sum(int a, int b)\n"
      "{\n"
      "	return a + b;\n"
      "}\n"
      "float sum(float a, float b)\n"
      "{\n"
      "	return a + b;\n"
      "}\n"
      "int main()\n"
      "{\n"
      "	sum(1, 2);\n"
      "	sum(1.0f, 0.5f);\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> int sum(int, int) <11:2 11:4>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> float sum(float, float) <12:2 12:4>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsFunctionCallInFunctionParameterList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int sum(int a, int b)\n"
      "{\n"
      "	return a + b;\n"
      "}\n"
      "int main()\n"
      "{\n"
      "	return sum(1, sum(2, 3));\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> int sum(int, int) <7:16 7:18>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsFunctionCallInMethod) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int sum(int a, int b)\n"
      "{\n"
      "	return a + b;\n"
      "}\n"
      "class App\n"
      "{\n"
      "	int main()\n"
      "	{\n"
      "		return sum(1, 2);\n"
      "	}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int App::main() -> int sum(int, int) <9:10 9:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsImplicitConstructorWithoutDefinitionCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	App app;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> void App::App() <6:6 6:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitConstructorCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "public:\n"
      "	App() {}\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	App();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> void App::App() <8:2 8:4>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCallOfExplicitlyDefinedDestructorAtDeleteKeyword) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Foo\n"
      "{\n"
      "public:\n"
      "	Foo() {}\n"
      "	~Foo() {}\n"
      "}; \n"
      "\n"
      "void foo()\n"
      "{\n"
      "	Foo* f = new Foo(); \n"
      "	delete f; \n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"void foo() -> void Foo::~Foo() <11:2 11:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCallOfImplicitlyDefinedDestructorAtDeleteKeyword) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Foo\n"
      "{\n"
      "}; \n"
      "\n"
      "void foo()\n"
      "{\n"
      "	Foo* f = new Foo(); \n"
      "	delete f; \n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"void foo() -> void Foo::~Foo() <8:2 8:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitConstructorCallOfField) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Item\n"
      "{\n"
      "};\n"
      "class App\n"
      "{\n"
      "public:\n"
      "	App() : item() {}\n"
      "	Item item;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"void App::App() -> void Item::Item() <7:10 7:13>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsFunctionCallInMemberInitialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int one() { return 1; }\n"
      "class Item\n"
      "{\n"
      "public:\n"
      "	Item(int n) {}\n"
      "};\n"
      "class App\n"
      "{\n"
      "	App()\n"
      "		: item(one())"
      "	{}\n"
      "	Item item;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"void App::App() -> int one() <10:10 10:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCopyConstructorCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "public:\n"
      "	App() {}\n"
      "	App(const App& other) {}\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	App app;\n"
      "	App app2(app);\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> void App::App(const App &) <10:6 10:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsGlobalVariableConstructorCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "public:\n"
      "	App() {}\n"
      "};\n"
      "App app;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"App app -> void App::App() <6:5 6:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsGlobalVariableFunctionCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int one() { return 1; }\n"
      "int a = one();\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int a -> int one() <2:9 2:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsOperatorCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "public:\n"
      "	void operator+(int a)\n"
      "	{\n"
      "	}\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	App app;\n"
      "	app + 2;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> void App::operator+(int) <11:6 11:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfFunctionPointer) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void my_int_func(int x)\n"
      "{\n"
      "}\n"
      "\n"
      "void test()\n"
      "{\n"
      "	void (*foo)(int);\n"
      "	foo = &my_int_func;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void test() -> void my_int_func(int) <8:9 8:19>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfGlobalVariableInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int bar;\n"
      "\n"
      "int main()\n"
      "{\n"
      "	bar = 1;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"int main() -> int bar <5:2 5:4>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfGlobalVariableInGlobalVariableInitialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int a = 0;\n"
      "int b[] = {a};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"int [] b -> int a <2:12 2:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfGlobalVariableInMethod) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int bar;\n"
      "\n"
      "class App\n"
      "{\n"
      "	void foo()\n"
      "	{\n"
      "		bar = 1;\n"
      "	}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void App::foo() -> int bar <7:3 7:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfFieldInMethod) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "	void foo()\n"
      "	{\n"
      "		bar = 1;\n"
      "		this->bar = 2;\n"
      "	}\n"
      "	int bar;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void App::foo() -> int App::bar <5:3 5:5>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void App::foo() -> int App::bar <6:9 6:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfFieldInFunctionCallArguments) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "public:\n"
      "	void foo(int i)\n"
      "	{\n"
      "		foo(bar);\n"
      "	}\n"
      "	int bar;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void A::foo(int) -> int A::bar <6:7 6:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfFieldInFunctionCallContext) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "public:\n"
      "	void foo(int i)\n"
      "	{\n"
      "		a->foo(6);\n"
      "	}\n"
      "	A* a;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void A::foo(int) -> A * A::a <6:3 6:3>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfFieldInInitializationList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "	App()\n"
      "		: bar(42)\n"
      "	{}\n"
      "	int bar;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void App::App() -> int App::bar <4:5 4:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfMemberInCallExpressionToUnresolvedMemberExpression) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	template <typename T>\n"
      "	T run() { return 5; }\n"
      "};\n"
      "class B {\n"
      "	template <typename T>\n"
      "	T run() {\n"
      "		return a.run<T>();\n"
      "	}\n"
      "	A a;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"T B::run<typename T>() -> A B::a <8:10 8:10>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfMemberInTemporaryObjectExpression) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Foo\n"
      "{\n"
      "public:\n"
      "	Foo() { }\n"
      "	Foo(const Foo& i, int d) { }\n"
      "};\n"
      "\n"
      "class Bar\n"
      "{\n"
      "public:\n"
      "	Bar(): m_i() {}\n"
      "\n"
      "	void baba()\n"
      "	{\n"
      "		Foo(m_i, 4);\n"
      "	}\n"
      "\n"
      "	const Foo m_i;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"void Bar::baba() -> const Foo Bar::m_i <15:7 15:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfMemberInDependentScopeMemberExpression) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T m_t;\n"
      "\n"
      "	void foo()\n"
      "	{\n"
      "		m_t.run();\n"
      "	}\n"
      "};\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->usages, L"void A<typename T>::foo() -> T A<typename T>::m_t <8:3 8:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsReturnTypeUseInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "double PI()\n"
      "{\n"
      "	return 3.14159265359;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"double PI() -> double <1:1 1:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsParameterTypeUsesInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void ceil(float a)\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void ceil(float) -> float <1:11 1:15>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUseOfDecayedParameterTypeInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template<class T, unsigned int N>\n"
      "class VectorBase\n"
      "{\n"
      "public:\n"
      "	VectorBase(T values[N]);\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:16> <5:13 5:13>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:32> <5:22 5:22>"));
}

TEST_F(CxxParserTestSuite, cxxParserUsageOfInjectedTypeInMethodDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class Foo\n"
      "{\n"
      "	Foo& operator=(const Foo&) = delete;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->typeUses,
      L"Foo<typename T> & Foo<typename T>::operator=(const Foo<typename T> &) -> Foo<typename T> "
      L"<4:2 4:4>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->typeUses,
      L"Foo<typename T> & Foo<typename T>::operator=(const Foo<typename T> &) -> Foo<typename T> "
      L"<4:23 4:25>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUseOfQualifiedTypeInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void test(const int t)\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void test(const int) -> int <1:17 1:19>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsParameterTypeUsesInConstructor) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	A(int a);\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void A::A(int) -> int <3:4 3:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeUsesInFunctionBody) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int main()\n"
      "{\n"
      "	int a = 42;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <3:2 3:4>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeUsesInMethodBody) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	int main()\n"
      "	{\n"
      "		int a = 42;\n"
      "		return a;\n"
      "	}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int A::main() -> int <5:3 5:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeUsesInLoopsAndConditions) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int main()\n"
      "{\n"
      "	if (true)\n"
      "	{\n"
      "		int a = 42;\n"
      "	}\n"
      "	for (int i = 0; i < 10; i++)\n"
      "	{\n"
      "		int b = i * 2;\n"
      "	}\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <5:3 5:5>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <7:7 7:9>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <9:3 9:5>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeUsesOfBaseClassInDerivedConstructor) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "public:\n"
      "	A(int n) {}\n"
      "};\n"
      "class B : public A\n"
      "{\n"
      "public:\n"
      "	B() : A(42) {}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void B::B() -> A <9:8 9:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsEnumUsesInGlobalSpace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum A\n"
      "{\n"
      "	B,\n"
      "	C\n"
      "};\n"
      "A a = B;\n"
      "A* aPtr = new A;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"A a -> A::B <6:7 6:7>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A a -> A <6:1 6:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A * aPtr -> A <7:1 7:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A * aPtr -> A <7:15 7:15>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsEnumUsesInFunctionBody) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum A\n"
      "{\n"
      "	B,\n"
      "	C\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A a = B;\n"
      "	A* aPtr = new A;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->usages, L"int main() -> A::B <8:8 8:8>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> A <8:2 8:2>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> A <9:2 9:2>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> A <9:16 9:16>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfTemplateParameterOfTemplateMemberVariableDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "struct IsBaseType {\n"
      "	static const bool value = true;\n"
      "};\n"
      "template <typename T>\n"
      "const bool IsBaseType<T>::value;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <1:20 1:20>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(    // TODO: fix FAIL because usage in name
                                                         // qualifier is not recorded
      client->localSymbols,
      L"temp.cpp<5:20> <5:20 5:20>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfTemplateParametersWithDifferentDepthOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	template <typename Q>\n"
      "	void foo(Q q)\n"
      "	{\n"
      "		T t;\n"
      "		t.run(q);\n"
      "	}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <7:3 7:3>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:21> <5:11 5:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfTemplateParametersWithDifferentDepthOfPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	template <typename Q, typename R>\n"
      "	class B\n"
      "	{\n"
      "		T foo(Q q, R r);\n"
      "	};\n"
      "\n"
      "	template <typename R>\n"
      "	class B<int, R>\n"
      "	{\n"
      "		T foo(R r);\n"
      "	};\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <13:3 13:3>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<10:21> <13:9 13:9>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsUsageOfTemplateTemplateParameterOfTemplateClassExplicitlyInstantiatedWithConcreteTypeArgument) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T>\n"
      "class B\n"
      "{\n"
      "	void foo(T<int> parameter)\n"
      "	{}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:36> <7:11 7:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfTemplateTemplateParameterOfTemplateClassExplicitlyInstantiatedWithTemplateType) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T>\n"
      "class B\n"
      "{\n"
      "	template <typename U> \n"
      "	void foo(T<U> parameter)\n"
      "	{}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:36> <8:11 8:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypedefInOtherClassThatDependsOnOwnTemplateParameter) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	typedef T type;\n"
      "};\n"
      "template <typename U>\n"
      "class B\n"
      "{\n"
      "public:\n"
      "	typedef typename A<U>::type type;\n"
      "};\n"
      "B<int>::type f = 0;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int>::type -> int <5:10 5:10>"));
  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->typeUses, L"B<typename U>::type -> A<typename T>::type <11:25 11:28>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"B<int>::type -> A<int>::type <11:25 11:28>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"B<int>::type f -> B<int>::type <13:9 13:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfTemplateParameterInQualifierOfOtherSymbol) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "struct find_if_impl;\n"
      "\n"
      "template <typename R, typename S>\n"
      "struct find_if\n"
      "{\n"
      "	template <typename... Ts>\n"
      "	using f = typename find_if_impl<S>::template f<R::template f, Ts...>;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:20> <8:49 8:49>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUseOfDependentTemplateSpecializationType) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	template <typename X>\n"
      "	using type = T;\n"
      "};\n"
      "template <typename U>\n"
      "class B\n"
      "{\n"
      "public:\n"
      "	typedef typename A<U>::template type<float> type;\n"
      "};\n"
      "B<bool>::type f = 0.0f;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->typeUses, L"B<typename U>::type -> A<typename T>::type<float> <12:10 12:17>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"B<bool>::type f -> B<bool>::type <14:10 14:13>"));
}

TEST_F(CxxParserTestSuite, cxxParserCreatesSingleNodeForAllPossibleParameterPackExpansionsOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template<typename T>\n"
      "T adder(T v) { return v; }\n"
      "\n"
      "template<typename T, typename... Args>\n"
      "T adder(T first, Args... args) { return first + adder(args...); }\n"
      "\n"
      "void foo() { long sum = adder(1, 2, 3, 8, 7); }\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->functions, L"int adder<int, <...>>(int, ...) <5:1 <5:1 <5:3 5:7> 5:30> 5:65>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->calls, L"int adder<int, <...>>(int, ...) -> int adder<int, <...>>(int, ...) <5:49 5:53>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A<int> a;\n"
      "	return 0;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> -> int <7:4 7:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <7:4 7:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentOfExplicitTemplateInstantiatedWithFunctionPrototype) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "void foo()\n"
      "{\n"
      "	A<int()> a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int()> -> int <7:4 7:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void foo() -> int <7:4 7:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void foo() -> int <7:4 7:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentForParameterPackOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename... T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "int main()\n"
      "{\n"
      "   A<int, float>();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<<...>> -> int <7:6 7:8>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<<...>> -> float <7:11 7:15>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <7:6 7:8>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> float <7:11 7:15>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <7:6 7:8>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> float <7:11 7:15>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentInNonDefaultConstructorOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	A(int data){}\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A<int>(5);\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> -> int <9:4 9:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <9:4 9:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <9:4 9:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentInDefaultConstructorOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	A(){}\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A<int>();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> -> int <9:4 9:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <9:4 9:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <9:4 9:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentInNewExpressionOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	A(){}\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	new A<int>();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> -> int <9:8 9:10>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <9:8 9:10>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <9:8 9:10>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoTemplateArgumentForBuiltinNonTypeIntTemplateParameterOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"    // use of "int"
      "class A\n"
      "{\n"
      "};\n"
      "int main()\n"    // use of "int"
      "{\n"
      "	A<1> a;\n"    // use of "A"
      "	return 0;\n"
      "}\n");

  EXPECT_TRUE(client->typeUses.size() == 3);
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoTemplateArgumentForBuiltinNonTypeBoolTemplateParameterOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T>\n"    // use of "bool"
      "class A\n"
      "{\n"
      "};\n"
      "int main()\n"    // use of "int"
      "{\n"
      "	A<true> a;\n"    // use of "A"
      "	return 0;\n"
      "}\n");

  EXPECT_TRUE(client->typeUses.size() == 3);
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeCustomPointerTemplateArgumentOfImplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P* p>\n"
      "class A\n"
      "{};\n"
      "P g_p;\n"
      "int main()\n"
      "{\n"
      "	A<&g_p> a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<&g_p> -> P g_p <9:5 9:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeCustomReferenceTemplateArgumentOfImplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P& p>\n"
      "class A\n"
      "{};\n"
      "P g_p;\n"
      "int main()\n"
      "{\n"
      "	A<g_p> a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<g_p> -> P g_p <9:4 9:6>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsNoTemplateArgumentForBuiltinNonTypeIntTemplateParameterPackOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int... T>\n"    // use of "int"
      "class A\n"
      "{\n"
      "};\n"
      "int main()\n"    // use of "int"
      "{\n"
      "   A<1, 2, 33>();\n"    // use of "A"
      "}\n");

  EXPECT_TRUE(client->typeUses.size() == 3);
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateTemplateArgumentOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T>\n"
      "class B\n"
      "{};\n"
      "int main()\n"
      "{\n"
      "	B<A> ba;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"B<A> -> A<typename T> <9:4 9:4>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> A<typename T> <9:4 9:4>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateTemplateArgumentForParameterPackOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <template<typename> typename... T>\n"
      "class B\n"
      "{\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	B<A, A>();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"B<<...>> -> A<typename T> <11:4 11:4>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"B<<...>> -> A<typename T> <11:7 11:7>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> A<typename T> <11:4 11:4>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> A<typename T> <11:7 11:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateArgumentForImplicitSpecializationOfGlobalTemplateVariable) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T v;\n"
      "void test()\n"
      "{\n"
      "	v<int> = 9;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int v<int> -> int <5:4 5:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void test() -> int <5:4 5:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateMemberSpecializationForMethodOfImplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	T foo() {}\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A<int> a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->templateSpecializations, L"int A<int>::foo() -> T A<typename T>::foo() <5:4 5:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateMemberSpecializationForStaticVariableOfImplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	static T foo;\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A<int> a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->templateSpecializations, L"static int A<int>::foo -> static T A<typename T>::foo <5:11 5:13>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateMemberSpecializationForFieldOfImplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	T foo;\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A<int> a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->templateSpecializations, L"int A<int>::foo -> T A<typename T>::foo <5:4 5:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateMemberSpecializationForFieldOfMemberClassOfImplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	class B {\n"
      "	public:\n"
      "		T foo;\n"
      "	};\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A<int>::B b;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->templateSpecializations, L"int A<int>::B::foo -> T A<typename T>::B::foo <7:5 7:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateMemberSpecializationForMemberClassOfImplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "public:\n"
      "	class B {};\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	A<int> a;\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->templateSpecializations, L"A<int>::B -> A<typename T>::B <5:8 5:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <>\n"
      "class A<int>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<int> -> int <6:9 6:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoTemplateArgumentForBuiltinNonTypeIntTemplateParameterOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"    // use of "int"
      "class A\n"
      "{\n"
      "};\n"
      "template <>\n"
      "class A<1>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(client->typeUses.size() == 1);
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoTemplateArgumentForBuiltinNonTypeBoolTemplateParameterOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T>\n"    // use of "bool"
      "class A\n"
      "{\n"
      "};\n"
      "template <>\n"
      "class A<true>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(client->typeUses.size() == 1);
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeCustomPointerTemplateArgumentOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P* p>\n"
      "class A\n"
      "{};\n"
      "P g_p;\n"
      "template <>\n"
      "class A<&g_p>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<&g_p> -> P g_p <8:10 8:12>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNonTypeCustomReferenceTemplateArgumentOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P& p>\n"
      "class A\n"
      "{};\n"
      "P g_p;\n"
      "template <>\n"
      "class A<g_p>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<g_p> -> P g_p <8:9 8:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateTemplateArgumentOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T>\n"
      "class B\n"
      "{};\n"
      "template <>\n"
      "class B<A>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"B<A> -> A<typename T> <8:9 8:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentsOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename U>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <typename T>\n"
      "class A<T, int>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<5:20> <6:9 6:9>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<typename T, int> -> int <6:12 6:14>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsNoTemplateArgumentForBuiltinNonTypeIntTemplateParameterOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T, int U>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <int U>\n"
      "class A<3, U>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<5:15> <6:12 6:12>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsNoTemplateArgumentForBuiltinNonTypeBoolTemplateParameterOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T, bool U>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <bool U>\n"
      "class A<true, U>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<5:16> <6:15 6:15>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsTemplateArgumentForNonTypeCustomPointerTemplateParameterOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P* p, P* q>\n"
      "class A\n"
      "{};\n"
      "P g_p;\n"
      "template <P* q>\n"
      "class A<&g_p, q>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->typeUses, L"A<&g_p, q> -> P g_p <8:10 8:12>"    // TODO: this is completely wrong?
                                                              // should be a normal usage
      ));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<7:14> <8:15 8:15>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsTemplateArgumentForNonTypeCustomReferenceTemplateParameterOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P& p, P& q>\n"
      "class A\n"
      "{};\n"
      "P g_p;\n"
      "template <P& q>\n"
      "class A<g_p, q>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<g_p, q> -> P g_p <8:9 8:11>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<7:14> <8:14 8:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateArgumentForTemplateTemplateParameterOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T, template<typename> class U>\n"
      "class B\n"
      "{};\n"
      "template <template<typename> class U>\n"
      "class B<A, U>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->typeUses, L"B<A, template<typename> typename U> -> A<typename T> <8:9 8:9>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<7:36> <8:12 8:12>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsNonTypeTemplateArgumentThatDependsOnTypeTemplateParameterOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T1, typename T2, T2 T3>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <typename T2, T2 T3>\n"
      "class A<3, T2, T3>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<5:27> <6:16 6:17>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsNonTypeTemplateArgumentThatDependsOnTemplateTemplateParameterOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T1, template<typename> class T2, T2<int> T3>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <template<typename> class T2, T2<int> T3>\n"
      "class A<3, T2, T3>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<5:36> <6:12 6:13>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<5:48> <6:16 6:17>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsImplicitTemplateClassSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T foo;\n"
      "};\n"
      "\n"
      "A<int> a;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->templateSpecializations, L"A<int> -> A<typename T> <2:7 2:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsClassInheritanceFromImplicitTemplateClassSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T foo;\n"
      "};\n"
      "\n"
      "class B: public A<int>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->inheritances, L"B -> A<int> <7:17 7:17>"));
}

// TODO(Hussein): Fix the test case
TEST_F(CxxParserTestSuite, DISABLED_recordBaseClassOfImplicitTemplateClassSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template<class T, unsigned int N>\n"
      "class VectorBase {}; \n"
      "\n"
      "template<class T>\n"
      "class Vector2 : public VectorBase<T, 2> { void foo(); }; \n"
      "\n"
      "typedef Vector2<float> Vec2f; \n"
      "\n"
      "Vec2f v; \n");

  EXPECT_THAT(client->inheritances, testing::Contains(testing::StrEq(L"Vector2<float> -> VectorBase<float, 2> <5:24 5:33>")));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateClassSpecializationWithTemplateArgument) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T foo;\n"
      "};\n"
      "\n"
      "template <typename U>\n"
      "class B: public A<U>\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<7:20> <8:19 8:19>"));
  EXPECT_TRUE(client->inheritances.size() == 1);
  EXPECT_TRUE(client->classes.size() == 2);
  EXPECT_TRUE(client->fields.size() == 1);
}

TEST_F(CxxParserTestSuite, cxxParserFindsCorrectOrderOfTemplateArgumentsForExplicitClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T1, typename T2, typename T3>\n"
      "class vector { };\n"
      "template<class Foo1, class Foo2>\n"
      "class vector<Foo2, Foo1, int> { };\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->classes, L"vector<class Foo2, class Foo1, int> <3:1 <4:7 4:12> 4:33>"));
}

TEST_F(CxxParserTestSuite, cxxParserReplacesDependentTemplateArgumentsOfExplicitTemplateSpecializationWithNameOfBaseTemplate) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A {};\n"
      "template <typename T1, typename T2>\n"
      "class vector { };\n"
      "template<class Foo1>\n"
      "class vector<Foo1, A<Foo1>> { };\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->classes, L"vector<class Foo1, A<typename T>> <5:1 <6:7 6:12> 6:31>"));
}

TEST_F(CxxParserTestSuite, cxxParserReplacesUnknownTemplateArgumentsOfExplicitTemplateSpecializationWithDepthAndPositionIndex) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T0>\n"
      "class foo {\n"
      "	template <typename T1, typename T2>\n"
      "	class vector { };\n"
      "	template<class T1>\n"
      "	class vector<T0, T1> { };\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->classes, L"foo<typename T0>::vector<arg0_0, class T1> <5:2 <6:8 6:13> 6:25>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateClassConstructorUsageOfField) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	A(): foo() {}\n"
      "	T foo;\n"
      "};\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->usages, L"void A<typename T>::A<T>() -> T A<typename T>::foo <4:7 4:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCorrectMethodReturnTypeOfTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T foo();\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<1:20> <4:2 4:2>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->methods, L"private T A<typename T>::foo() <4:2 <4:4 4:6> 4:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateDefaultArgumentTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T = int>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"A<typename T> -> int <1:24 1:26>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoDefaultArgumentTypeForNonTypeBoolTemplateParameterOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T = true>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(client->typeUses.size() == 1);
  ;    // only the "bool" type is recorded and nothing for the default arg
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateTemplateDefaultArgumentTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T = A>\n"
      "class B\n"
      "{};\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->typeUses, L"B<template<typename> typename T> -> A<typename T> <4:40 4:40>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsImplicitInstantiationOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(T a)\n"
      "{\n"
      "	return a;\n"
      "};\n"
      "\n"
      "int main()\n"
      "{\n"
      "	return test(1);\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->templateSpecializations, L"int test<int>(int) -> T test<typename T>(T) <2:3 2:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitSpecializationOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(T a)\n"
      "{\n"
      "	return a;\n"
      "};\n"
      "\n"
      "template <>\n"
      "int test<int>(int a)\n"
      "{\n"
      "	return a + a;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->templateSpecializations, L"int test<int>(int) -> T test<typename T>(T) <8:5 8:8>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitTypeTemplateArgumentOfExplicitInstantiationOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "void test()\n"
      "{\n"
      "};\n"
      "\n"
      "template <>\n"
      "void test<int>()\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void test<int>() -> int <7:11 7:13>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitTypeTemplateArgumentOfFunctionCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "void test(){}\n"
      "\n"
      "int main()\n"
      "{\n"
      "	test<int>();\n"
      "	return 1;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void test<int>() -> int <6:7 6:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoExplicitNonTypeIntTemplateArgumentOfFunctionCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"    // use of "int"
      "void test(){}\n"       // 2x use of "void"
      "\n"
      "int main()\n"    // use of "int"
      "{\n"
      "	test<33>();\n"
      "	return 1;\n"
      "};\n");

  EXPECT_TRUE(client->typeUses.size() == 4);
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitTemplateTemplateArgumentOfFunctionCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A {};\n"
      "template <template<typename> class T>\n"
      "void test(){};\n"
      "int main()\n"
      "{\n"
      "	test<A>();\n"
      "	return 1;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void test<A>() -> A<typename T> <7:7 7:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoImplicitTypeTemplateArgumentOfFunctionCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "void test(T data){}\n"    // 2x use of "void" + 1x use of "int"
      "\n"
      "int main()\n"    // use of "int"
      "{\n"
      "	test(1);\n"
      "	return 1;\n"
      "};\n");

  EXPECT_TRUE(client->typeUses.size() == 4);
}

TEST_F(CxxParserTestSuite, cxxParserFindsExplicitTypeTemplateArgumentOfFunctionCallInVarDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(){ return 1; }\n"
      "\n"
      "class A\n"
      "{\n"
      "	int foo = test<int>();\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int test<int>() -> int <6:17 6:19>"));
}

TEST_F(CxxParserTestSuite, CxxParserFindsNoImplicitTypeTemplateArgumentOfFunctionCallInVarDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(T i){ return i; }\n"    // 2x use of "int"
      "\n"
      "class A\n"
      "{\n"
      "	int foo = test(1);\n"    // usage of "int"
      "};\n");

  EXPECT_TRUE(client->typeUses.size() == 3);
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateDefaultArgumentTypeOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T = int>\n"
      "void test()\n"
      "{\n"
      "};\n"
      "\n"
      "int main()\n"
      "{\n"
      "	test();\n"
      "	return 1;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"void test<typename T>() -> int <1:24 1:26>"));
}

TEST_F(CxxParserTestSuite, cxxParserDoesNotFindDefaultArgumentTypeForNonTypeBoolTemplateParameterOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T = true>\n"
      "void test()\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(client->typeUses.size() == 2);
  ;    // only "bool" and "void" is recorded
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateTemplateDefaultArgumentTypeOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T = A>\n"
      "void test()\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->typeUses, L"void test<template<typename> typename T>() -> A<typename T> <4:40 4:40>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsLambdaCallingAFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void func() {}\n"
      "void lambdaCaller()\n"
      "{\n"
      "	[]()\n"
      "	{\n"
      "		func();\n"
      "	}();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(
      client->calls, L"void lambdaCaller::lambda at 4:2() const -> void func() <6:3 6:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsLocalVariableInLambdaCapture) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaWrapper()\n"
      "{\n"
      "	int x = 2;\n"
      "	[x]() { return 1; }();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:6> <4:3 4:3>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsUsageOfLocalVariableInMicrosoftInlineAssemblyStatement) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void foo()\n"
      "{\n"
      "	int x = 2;\n"
      "__asm\n"
      "{\n"
      "	mov eax, x\n"
      "	mov x, eax\n"
      "}\n"
      "}\n",
      {L"--target=i686-pc-windows-msvc"});

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:6> <6:11 6:11>"));

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:6> <7:6 7:6>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTemplateArgumentOfUnresolvedLookupExpression) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "void a()\n"
      "{\n"
      "}\n"
      "\n"
      "template <typename MessageType>\n"
      "void dispatch()\n"
      "{\n"
      "	a<MessageType>();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<6:20> <9:4 9:14>"));
}

///////////////////////////////////////////////////////////////////////////////
// test finding symbol locations

TEST_F(CxxParserTestSuite, cxxParserFindsCorrectLocationOfExplicitConstructorDefinedInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace n\n"
      "{\n"
      "	class App\n"
      "	{\n"
      "	public:\n"
      "		App(int i) {}\n"
      "	};\n"
      "}\n"
      "int main()\n"
      "{\n"
      "	n::App a = n::App(2);\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int main() -> void n::App::App(int) <11:16 11:18>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroArgumentLocationForFieldDefinitionWithNamePassedAsArgumentToMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define DEF_INT_FIELD(name) int name;\n"
      "class A {\n"
      "	DEF_INT_FIELD(m_value)\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->fields, L"private int A::m_value <3:16 3:22>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroUsageLocationForFieldDefinitionWithNamePartiallyPassedAsArgumentToMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define DEF_INT_FIELD(name) int m_##name;\n"
      "class A {\n"
      "	DEF_INT_FIELD(value)\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->fields, L"private int A::m_value <3:2 3:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroArgumentLocationForFunctionCallInCodePassedAsArgumentToMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define DEF_INT_FIELD(name, init) int name = init;\n"
      "int foo() { return 5; }\n"
      "class A {\n"
      "	DEF_INT_FIELD(m_value, foo())\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int A::m_value -> int foo() <4:25 4:27>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsMacroUsageLocationForFunctionCallInCodeOfMacroBody) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int foo() { return 5; }\n"
      "#define DEF_INT_FIELD(name) int name = foo();\n"
      "class A {\n"
      "	DEF_INT_FIELD(m_value)\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"int A::m_value -> int foo() <4:2 4:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsTypeTemplateArgumentOfStaticCastExpression) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int main()\n"
      "{\n"
      "	return static_cast<int>(4.0f);"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->typeUses, L"int main() -> int <3:21 3:23>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsImplicitConstructorCallInInitialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "};\n"
      "class B\n"
      "{\n"
      "	B(){}\n"
      "	A m_a;\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->calls, L"void B::B() -> void A::A() <6:2 6:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserParsesMultipleFiles) {
  const std::set<FilePath> indexedPaths = {FilePath(L"data/CxxParserTestSuite/")};
  const std::set<FilePathFilter> excludeFilters;
  const std::set<FilePathFilter> includeFilters;
  const FilePath workingDirectory(L".");
  const FilePath sourceFilePath(L"data/CxxParserTestSuite/code.cpp");

  std::shared_ptr<IndexerCommandCxx> indexerCommand = std::make_shared<IndexerCommandCxx>(
      sourceFilePath,
      indexedPaths,
      excludeFilters,
      includeFilters,
      workingDirectory,
      std::vector<std::wstring> {L"--target=x86_64-pc-windows-msvc", L"-std=c++1z", sourceFilePath.wstr()});

  std::shared_ptr<IntermediateStorage> storage = std::make_shared<IntermediateStorage>();
  CxxParser parser(std::make_shared<ParserClientImpl>(storage.get()),
                   std::make_shared<TestFileRegister>(),
                   std::make_shared<IndexerStateInfo>());

  parser.buildIndex(indexerCommand);

  std::shared_ptr<TestStorage> testStorage = TestStorage::create(storage);

  EXPECT_TRUE(testStorage->errors.size() == 0);

  EXPECT_TRUE(testStorage->typedefs.size() == 1);
  EXPECT_TRUE(testStorage->classes.size() == 4);
  EXPECT_TRUE(testStorage->enums.size() == 1);
  EXPECT_TRUE(testStorage->enumConstants.size() == 2);
  EXPECT_TRUE(testStorage->functions.size() == 2);
  EXPECT_TRUE(testStorage->fields.size() == 4);
  EXPECT_TRUE(testStorage->globalVariables.size() == 2);
  EXPECT_TRUE(testStorage->methods.size() == 15);
  EXPECT_TRUE(testStorage->namespaces.size() == 2);
  EXPECT_TRUE(testStorage->structs.size() == 1);

  EXPECT_TRUE(testStorage->inheritances.size() == 1);
  EXPECT_TRUE(testStorage->calls.size() == 3);
  EXPECT_TRUE(testStorage->usages.size() == 3);
  EXPECT_TRUE(testStorage->typeUses.size() == 16);

  EXPECT_TRUE(testStorage->files.size() == 2);
  EXPECT_TRUE(testStorage->includes.size() == 1);
}

TEST_F(CxxParserTestSuite, cxxParserFindsBracesOfClassDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<2:1> <2:1 2:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<2:1> <3:1 3:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsBracesOfNamespaceDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace n\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<2:1> <2:1 2:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<2:1> <3:1 3:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsBracesOfFunctionDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int main()\n"
      "{\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<2:1> <2:1 2:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<2:1> <3:1 3:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsBracesOfMethodDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "public:\n"
      "	App(int i) {}\n"
      "};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:13> <4:13 4:13>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:13> <4:14 4:14>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsBracesOfInitList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int a = 0;\n"
      "int b[] = {a};\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<2:11> <2:11 2:11>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<2:11> <2:13 2:13>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsBracesOfLambda) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaCaller()\n"
      "{\n"
      "	[](){}();\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:6> <3:6 3:6>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:6> <3:7 3:7>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsBracesOfAsmStmt) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void foo()\n"
      "{\n"
      "	__asm\n"
      "	{\n"
      "		mov eax, eax\n"
      "	}\n"
      "}\n",
      {L"--target=i686-pc-windows-msvc"});

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:2> <4:2 4:2>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<4:2> <6:2 6:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsNoDuplicateBracesOfTemplateClassAndMethodDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class App\n"
      "{\n"
      "public:\n"
      "	App(int i) {}\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	App<int> a;\n"
      "	return 0;\n"
      "}\n");

  EXPECT_TRUE(client->localSymbols.size() == 9);
  ;    // 8 braces + 1 template parameter
}

TEST_F(CxxParserTestSuite, cxxParserFindsBracesWithClosingBracketInMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "\n"
      "namespace constants\n"
      "{\n"
      "\n"
      "#define CONSTANT(name, x)	\\\n"
      "	int name = x;			\\\n"
      "	} namespace constants {\n"
      "\n"
      "CONSTANT(half, 5)\n"
      "CONSTANT(third, 3)\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:1> <3:1 3:1>"));
  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<3:1> <7:2 7:2>"));
  // TS_ASSERT(utility::containsElement<std::wstring>(client->localSymbols, L"<0:0> <11:1
  // 11:1>")); // unwanted sideeffect

  client = parseCode(
      "\n"
      "#define CONSTANT(name, x)\\\n"
      "	int name = x;\\\n"
      "	} namespace constants {\n"
      "\n"
      "namespace constants\n"
      "{\n"
      "CONSTANT(half, 5)\n"
      "CONSTANT(third, 3)\n"
      "}\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<7:1> <7:1 7:1>"));
  // TS_ASSERT(utility::containsElement<std::wstring>(client->localSymbols, L"temp.cpp<7:1> <10:1
  // 10:1>")); // missing TS_ASSERT(utility::containsElement<std::wstring>(client->localSymbols,
  // L"<0:0> <10:1 10:1>")); // unwanted sideeffect
}

TEST_F(CxxParserTestSuite, cxxParserFindsCorrectSignatureLocationOfConstructorWithInitializerList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	A(const int& foo) : m_foo(foo)\n"
      "	{\n"
      "	}\n"
      "	const int m_foo\n"
      "}\n");
  ;

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->methods, L"private void A::A(const int &) <3:2 <3:2 <3:2 3:2> 3:18> 5:2>"));
}

TEST_F(CxxParserTestSuite, cxxParserCatchesError) {
  std::shared_ptr<TestStorage> client = parseCode("int a = b;\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->errors, L"use of undeclared identifier \'b\' <1:9 1:9>"));
}

TEST_F(CxxParserTestSuite, cxxParserCatchesErrorInForceInclude) {
  std::shared_ptr<TestStorage> client = parseCode("void foo() {} \n", {L"-include nothing"});

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->errors, L"' nothing' file not found <1:1 1:1>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsCorrectErrorLocationAfterLineDirective) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#line 55 \"foo.hpp\"\n"
      "void foo()\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->errors, L"expected function body after function declarator <2:11 2:11>"));
}

TEST_F(CxxParserTestSuite, cxxParserCatchesErrorInMacroExpansion) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define MACRO_WITH_NONEXISTENT_PATH \"this_path_does_not_exist.txt\"\n"
      "#include MACRO_WITH_NONEXISTENT_PATH\n");

  EXPECT_TRUE(
      utility::containsElement<std::wstring>(client->errors, L"'this_path_does_not_exist.txt' file not found <2:10 2:10>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsLocationOfLineComment) {
  std::shared_ptr<TestStorage> client = parseCode("// this is a line comment\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->comments, L"comment <1:1 1:26>"));
}

TEST_F(CxxParserTestSuite, cxxParserFindsLocationOfBlockComment) {
  std::shared_ptr<TestStorage> client = parseCode(
      "/* this is a\n"
      "block comment */\n");

  EXPECT_TRUE(utility::containsElement<std::wstring>(client->comments, L"comment <1:1 2:17>"));
}

void _test_TEST() {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<template<typename> class> class T>\n"
      "class A {\n"
      "T<>\n"
      "};\n"
      "template <template<typename> class T>\n"
      "class B {};\n"
      "template <typename T>\n"
      "class C {};\n"
      "A<B> a;\n");
  [[maybe_unused]] int ofo = 0;
}