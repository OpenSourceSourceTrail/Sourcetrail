#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../../src/lib/tests/mocks/MockedApplicationSetting.hpp"
#include "CxxParser.h"
#include "IndexerCommandCxx.h"
#include "IndexerStateInfo.h"
#include "language_packages.h"
#include "ParserClientImpl.h"
#include "TestFileRegister.h"
#include "TestStorage.h"
#include "TextAccess.h"
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

TEST_F(CxxParserTestSuite, FindsGlobalVariableDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("int x;\n");

  EXPECT_THAT(client->globalVariables, testing::Contains(L"int x <1:5 1:5>"));
}

TEST_F(CxxParserTestSuite, FindsStaticGlobalVariableDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("static int x;\n");

  EXPECT_THAT(client->globalVariables, testing::Contains(L"int x (temp.cpp) <1:12 1:12>"));
}

TEST_F(CxxParserTestSuite, FindsStaticConstGlobalVariableDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("static const int x;\n");

  EXPECT_THAT(client->globalVariables, testing::Contains(L"const int x (temp.cpp) <1:18 1:18>"));
}

TEST_F(CxxParserTestSuite, FindsGlobalClassDefinition) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->classes, testing::Contains(L"A <1:1 <1:7 1:7> 3:1>"));
}

TEST_F(CxxParserTestSuite, FindsGlobalClassDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("class A;\n");

  EXPECT_THAT(client->classes, testing::Contains(L"A <1:7 1:7>"));
}

TEST_F(CxxParserTestSuite, FindsGlobalStructDefinition) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->structs, testing::Contains(L"A <1:1 <1:8 1:8> 3:1>"));
}

TEST_F(CxxParserTestSuite, FindsGlobalStructDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode("struct A;\n");

  EXPECT_THAT(client->structs, testing::Contains(L"A <1:8 1:8>"));
}

TEST_F(CxxParserTestSuite, FindsVariableDefinitionsInGlobalScope) {
  std::shared_ptr<TestStorage> client = parseCode("int x;\n");

  EXPECT_THAT(client->globalVariables, testing::Contains(L"int x <1:5 1:5>"));
}

TEST_F(CxxParserTestSuite, FindsFieldsInClassWithAccessType) {
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

  EXPECT_THAT(client->fields, testing::Contains(L"private int A::a <3:6 3:6>"));
  EXPECT_THAT(client->fields, testing::Contains(L"public int A::b <6:6 6:6>"));
  EXPECT_THAT(client->fields, testing::Contains(L"protected static int A::c <8:13 8:13>"));
  EXPECT_THAT(client->fields, testing::Contains(L"private const int A::d <10:12 10:12>"));
}

TEST_F(CxxParserTestSuite, FindsFunctionDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int ceil(float a)\n"
      "{\n"
      "	return 1;\n"
      "}\n");

  EXPECT_THAT(client->functions, testing::Contains(testing::StrEq(L"int ceil(float) <1:1 <1:1 <1:5 1:8> 1:17> 4:1>")));
}

TEST_F(CxxParserTestSuite, FindsStaticFunctionDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "static int ceil(float a)\n"
      "{\n"
      "	return static_cast<int>(a) + 1;\n"
      "}\n");

  EXPECT_THAT(client->functions,
              testing::Contains(testing::StrEq(L"static int ceil(float) (temp.cpp) <1:1 <1:1 <1:12 1:15> 1:24> 4:1>")));
}

TEST_F(CxxParserTestSuite, FindsMethodDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	B();\n"
      "};\n");

  EXPECT_THAT(client->methods, testing::Contains(testing::StrEq(L"public void B::B() <4:2 <4:2 4:2> 4:4>")));
}

TEST_F(CxxParserTestSuite, FindsOverloadedOperatorDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	B& operator=(const B& other);\n"
      "};\n");

  EXPECT_THAT(client->methods, testing::Contains(L"public B & B::operator=(const B &) <4:2 <4:5 4:13> 4:29>"));
}

TEST_F(CxxParserTestSuite, FindsMethodDeclarationAndDefinition) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	B();\n"
      "};\n"
      "B::B()\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->methods, testing::Contains(L"public void B::B() <6:1 <6:4 6:4> 8:1>"));
}

TEST_F(CxxParserTestSuite, FindsVirtualMethodDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	virtual void process();\n"
      "};\n");

  EXPECT_THAT(client->methods, testing::Contains(L"public void B::process() <4:2 <4:15 4:21> 4:23>"));
}

TEST_F(CxxParserTestSuite, FindsPureVirtualMethodDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "protected:\n"
      "	virtual void process() = 0;\n"
      "};\n");

  EXPECT_THAT(client->methods, testing::Contains(L"protected void B::process() <4:2 <4:15 4:21> 4:27>"));
}

TEST_F(CxxParserTestSuite, FindsNamedNamespaceDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace A\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->namespaces, testing::Contains(L"A <1:1 <1:11 1:11> 3:1>"));
}

TEST_F(CxxParserTestSuite, FindsAnonymousNamespaceDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->namespaces, testing::Contains(L"anonymous namespace (temp.cpp<1:1>) <1:1 <2:1 2:1> 3:1>"));
}

TEST_F(CxxParserTestSuite, FindsMultipleAnonymousNamespaceDeclarationsAsSameSymbol) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "}\n"
      "namespace\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->namespaces, testing::Contains(L"anonymous namespace (temp.cpp<1:1>) <1:1 <2:1 2:1> 3:1>"));

  EXPECT_THAT(client->namespaces, testing::Contains(L"anonymous namespace (temp.cpp<1:1>) <4:1 <5:1 5:1> 6:1>"));
}

TEST_F(CxxParserTestSuite, FindsMultipleNestedAnonymousNamespaceDeclarationsAsDifferentSymbol) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "	namespace\n"
      "	{\n"
      "	}\n"
      "}\n");

  EXPECT_THAT(client->namespaces, testing::Contains(L"anonymous namespace (temp.cpp<1:1>) <1:1 <2:1 2:1> 6:1>"));

  EXPECT_THAT(client->namespaces,
              testing::Contains(L"anonymous namespace (temp.cpp<1:1>)::anonymous namespace (temp.cpp<3:2>) <3:2 <4:2 4:2> "
                                L"5:2>"));
}

TEST_F(CxxParserTestSuite, FindsAnonymousNamespaceDeclarationsNestedInsideNamespacesWithDifferentNameAsDifferentSymbol) {
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

  EXPECT_THAT(client->namespaces, testing::Contains(L"a <1:1 <1:11 1:11> 6:1>"));

  EXPECT_THAT(client->namespaces, testing::Contains(L"a::anonymous namespace (temp.cpp<3:2>) <3:2 <4:2 4:2> 5:2>"));

  EXPECT_THAT(client->namespaces, testing::Contains(L"b <7:1 <7:11 7:11> 12:1>"));

  EXPECT_THAT(client->namespaces, testing::Contains(L"b::anonymous namespace (temp.cpp<9:2>) <9:2 <10:2 10:2> 11:2>"));
}

TEST_F(CxxParserTestSuite, FindsAnonymousNamespaceDeclarationsNestedInsideNamespacesWithSameNameAsSameSymbol) {
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

  EXPECT_THAT(client->namespaces, testing::Contains(L"a <1:1 <1:11 1:11> 6:1>"));

  EXPECT_THAT(client->namespaces, testing::Contains(L"a::anonymous namespace (temp.cpp<3:2>) <3:2 <4:2 4:2> 5:2>"));

  EXPECT_THAT(client->namespaces, testing::Contains(L"a <7:1 <7:11 7:11> 12:1>"));

  EXPECT_THAT(client->namespaces, testing::Contains(L"a::anonymous namespace (temp.cpp<3:2>) <9:2 <10:2 10:2> 11:2>"));
}

TEST_F(CxxParserTestSuite, FindsAnonymousStructDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef struct\n"
      "{\n"
      "	int x;\n"
      "};\n");

  EXPECT_THAT(client->structs, testing::Contains(L"anonymous struct (temp.cpp<1:9>) <1:9 <1:9 1:14> 4:1>"));
}

TEST_F(CxxParserTestSuite, FindsMultipleAnonymousStructDeclarationsAsDistinctElements) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef struct\n"
      "{\n"
      "	int x;\n"
      "};\n"
      "typedef struct\n"
      "{\n"
      "	float x;\n"
      "};\n");

  EXPECT_EQ(client->structs.size(), 2);
  EXPECT_EQ(client->fields.size(), 2);
  EXPECT_TRUE(utility::substrBeforeLast(client->fields[0], '<') != utility::substrBeforeLast(client->fields[1], '<'));
}

TEST_F(CxxParserTestSuite, FindsAnonymousUnionDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef union\n"
      "{\n"
      "	int i;\n"
      "	float f;\n"
      "};\n");

  EXPECT_THAT(client->unions, testing::Contains(L"anonymous union (temp.cpp<1:9>) <1:9 <1:9 1:13> 5:1>"));
}

TEST_F(CxxParserTestSuite, FindsNameOfAnonymousStructDeclaredInsideTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef struct\n"
      "{\n"
      "	int x;\n"
      "} Foo;\n");

  EXPECT_THAT(client->structs, testing::Contains(L"Foo <1:9 <1:9 1:14> 4:1>"));
  EXPECT_THAT(client->structs, testing::Contains(L"Foo <4:3 4:5>"));
}

TEST_F(CxxParserTestSuite, FindsNameOfAnonymousClassDeclaredInsideTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef class\n"
      "{\n"
      "	int x;\n"
      "} Foo;\n");

  EXPECT_THAT(client->classes, testing::Contains(L"Foo <1:9 <1:9 1:13> 4:1>"));
  EXPECT_THAT(client->classes, testing::Contains(L"Foo <4:3 4:5>"));
}

TEST_F(CxxParserTestSuite, FindsNameOfAnonymousEnumDeclaredInsideTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef enum\n"
      "{\n"
      "	CONSTANT_1;\n"
      "} Foo;\n");

  EXPECT_THAT(client->enums, testing::Contains(L"Foo <1:9 <1:9 1:12> 4:1>"));
  EXPECT_THAT(client->enums, testing::Contains(L"Foo <4:3 4:5>"));
}

TEST_F(CxxParserTestSuite, FindsNameOfAnonymousUnionDeclaredInsideTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef union\n"
      "{\n"
      "	int x;\n"
      "	float y;\n"
      "} Foo;\n");

  EXPECT_THAT(client->unions, testing::Contains(L"Foo <1:9 <1:9 1:13> 5:1>"));
  EXPECT_THAT(client->unions, testing::Contains(L"Foo <5:3 5:5>"));
}

TEST_F(CxxParserTestSuite, FindsNameOfAnonymousStructDeclaredInsideTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "using Foo = struct\n"
      "{\n"
      "	int x;\n"
      "};\n");

  EXPECT_THAT(client->structs, testing::Contains(L"Foo <1:13 <1:13 1:18> 4:1>"));
  EXPECT_THAT(client->structs, testing::Contains(L"Foo <1:7 1:9>"));
}

TEST_F(CxxParserTestSuite, FindsNameOfAnonymousClassDeclaredInsideTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "using Foo = class\n"
      "{\n"
      "	int x;\n"
      "};\n");

  EXPECT_THAT(client->classes, testing::Contains(L"Foo <1:13 <1:13 1:17> 4:1>"));
  EXPECT_THAT(client->classes, testing::Contains(L"Foo <1:7 1:9>"));
}

TEST_F(CxxParserTestSuite, FindsNameOfAnonymousEnumDeclaredInsideTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "using Foo = enum\n"
      "{\n"
      "	CONSTANT_1;\n"
      "};\n");

  EXPECT_THAT(client->enums, testing::Contains(L"Foo <1:13 <1:13 1:16> 4:1>"));
  EXPECT_THAT(client->enums, testing::Contains(L"Foo <1:7 1:9>"));
}

TEST_F(CxxParserTestSuite, FindsNameOfAnonymousUnionDeclaredInsideTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "using Foo = union\n"
      "{\n"
      "	int x;\n"
      "	float y;\n"
      "};\n");

  EXPECT_THAT(client->unions, testing::Contains(L"Foo <1:13 <1:13 1:17> 5:1>"));
  EXPECT_THAT(client->unions, testing::Contains(L"Foo <1:7 1:9>"));
}

TEST_F(CxxParserTestSuite, FindsEnumDefinedInGlobalNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum E\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->enums, testing::Contains(L"E <1:1 <1:6 1:6> 3:1>"));
}

TEST_F(CxxParserTestSuite, FindsEnumConstantInGlobalEnum) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum E\n"
      "{\n"
      "	P\n"
      "};\n");

  EXPECT_THAT(client->enumConstants, testing::Contains(L"E::P <3:2 3:2>"));
}

TEST_F(CxxParserTestSuite, FindsTypedefInGlobalNamespace) {
  std::shared_ptr<TestStorage> client = parseCode("typedef unsigned int uint;\n");

  EXPECT_THAT(client->typedefs, testing::Contains(L"uint <1:22 1:25>"));
}

TEST_F(CxxParserTestSuite, FindsTypedefInNamedNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace test\n"
      "{\n"
      "	typedef unsigned int uint;\n"
      "}\n");

  EXPECT_THAT(client->typedefs, testing::Contains(L"test::uint <3:23 3:26>"));
}

TEST_F(CxxParserTestSuite, FindsTypedefInAnonymousNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "	typedef unsigned int uint;\n"
      "}\n");

  EXPECT_THAT(client->typedefs, testing::Contains(L"anonymous namespace (temp.cpp<1:1>)::uint <3:23 3:26>"));
}

TEST_F(CxxParserTestSuite, FindsTypeAliasInClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class Foo\n"
      "{\n"
      "	using Bar = Foo;\n"
      "};\n");

  EXPECT_THAT(client->typedefs, testing::Contains(L"private Foo::Bar <3:8 3:10>"));
}

TEST_F(CxxParserTestSuite, FindsMacroDefine) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI\n"
      "void test()\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->macros, testing::Contains(L"PI <1:9 1:10>"));
}

TEST_F(CxxParserTestSuite, FindsMacroUndefine) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#undef PI\n"
      "void test()\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->macroUses, testing::Contains(L"temp.cpp -> PI <1:8 1:9>"));
}

TEST_F(CxxParserTestSuite, FindsMacroInIfdef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI\n"
      "#ifdef PI\n"
      "void test()\n"
      "{\n"
      "};\n"
      "#endif\n");

  EXPECT_THAT(client->macroUses, testing::Contains(L"temp.cpp -> PI <2:8 2:9>"));
}

TEST_F(CxxParserTestSuite, FindsMacroInIfndef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI\n"
      "#ifndef PI\n"
      "void test()\n"
      "{\n"
      "};\n"
      "#endif\n");

  EXPECT_THAT(client->macroUses, testing::Contains(L"temp.cpp -> PI <2:9 2:10>"));
}

TEST_F(CxxParserTestSuite, FindsMacroInIfdefined) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI\n"
      "#if defined(PI)\n"
      "void test()\n"
      "{\n"
      "};\n"
      "#endif\n");

  EXPECT_THAT(client->macroUses, testing::Contains(L"temp.cpp -> PI <2:13 2:14>"));
}

TEST_F(CxxParserTestSuite, FindsMacroExpand) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI 3.14159265359\n"
      "void test()\n"
      "{\n"
      "double i = PI;"
      "};\n");

  EXPECT_THAT(client->macroUses, testing::Contains(L"temp.cpp -> PI <4:12 4:13>"));
}

TEST_F(CxxParserTestSuite, FindsMacroExpandWithinMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define PI 3.14159265359\n"
      "#define TAU (2 * PI)\n"
      "void test()\n"
      "{\n"
      "double i = TAU;"
      "};\n");

  EXPECT_THAT(client->macroUses, testing::Contains(L"temp.cpp -> PI <2:18 2:19>"));
}

TEST_F(CxxParserTestSuite, FindsMacroDefineScope) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define MAX(a,b) \\\n"
      "	((a)>(b)?(a):(b))");

  EXPECT_THAT(client->macros, testing::Contains(L"MAX <1:9 <1:9 1:11> 2:17>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterDefinitionOfTemplateTypeAlias) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template<class T>\n"
      "using MyType = int;\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:16> <1:16 1:16>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterDefinitionOfClassTemplate) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <1:20 1:20>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterDefinitionOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename U>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <typename T>\n"
      "class A<T, int>\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<5:20> <5:20 5:20>"));

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<5:20> <6:9 6:9>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterDefinitionOfVariableTemplate) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T v;\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <1:20 1:20>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterDefinitionOfExplicitPartialVariableTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename Q>\n"
      "T t = Q(5);\n"
      "\n"
      "template <typename R>\n"
      "int t<int, R> = 9;\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:20> <4:20 4:20>"));

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:20> <5:12 5:12>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterDefinedWithClassKeyword) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <class T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:17> <1:17 1:17>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeIntTemplateParameterDefinitionOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:15> <1:15 1:15>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeBoolTemplateParameterDefinitionOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:16> <1:16 1:16>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeCustomPointerTemplateParameterDefinitionOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P* p>\n"
      "class A\n"
      "{};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:14> <3:14 3:14>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeCustomReferenceTemplateParameterDefinitionOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P& p>\n"
      "class A\n"
      "{};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:14> <3:14 3:14>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeTemplateParameterDefinitionThatDependsOnTypeTemplateParameterOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T1, T1& T2>\n"
      "class A\n"
      "{};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:28> <1:28 1:29>"));

  // and usage
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <1:24 1:25>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeTemplateParameterDefinitionThatDependsOnTemplateTemplateParameterOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<typename> class T1, T1<int>& T2>\n"
      "class A\n"
      "{};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:49> <1:49 1:50>"));

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:36> <1:40 1:41>"));
}

TEST_F(CxxParserTestSuite,
       cxxParserFindsNonTypeTemplateParameterDefinitionThatDependsOnTypeTemplateParameterOfTemplateTemplateParameter) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<typename T, T R>typename S>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:45> <1:45 1:45>"));

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:29> <1:32 1:32>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateArgumentOfDependentNonTypeTemplateParameter) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<typename> class T1, T1<int>& T2>\n"
      "class A\n"
      "{};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<template<typename> typename T1, T1<int> & T2> -> int <1:43 1:45>"));
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

TEST_F(CxxParserTestSuite, FindsTemplateTemplateParameterOfTemplateClass) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:36> <4:36 4:36>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterPackTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename... T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:23> <1:23 1:23>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeIntTemplateParameterPackTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int... T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:18> <1:18 1:18>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateTemplateParameterPackTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <template<typename> typename... T>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:42> <1:42 1:42>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParametersOfTemplateClassWithMultipleParameters) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename U>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <1:20 1:20>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:32> <1:32 1:32>"));
}

TEST_F(CxxParserTestSuite, SkipsCreatingNodeForTemplateParameterWithoutAName) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename>\n"
      "class A\n"
      "{\n"     // local symbol for brace
      "};\n"    // local symbol for brace
  );

  EXPECT_EQ(client->localSymbols.size(), 2);
  EXPECT_THAT(client->classes, testing::Contains(L"A<typename> <1:1 <2:7 2:7> 4:1>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterOfTemplateMethodDefinitionOutsideTemplateClass) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<8:20> <8:20 8:20>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <>\n"
      "class A<int>\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->classes, testing::Contains(L"A<int> <5:1 <6:7 6:7> 8:1>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitVariableTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T t = T(5);\n"
      "\n"
      "template <>\n"
      "int t<int> = 99;\n");

  EXPECT_THAT(client->globalVariables, testing::Contains(L"int t<int> <5:5 5:5>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename U>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <typename T>\n"
      "class A<T, int>\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->classes, testing::Contains(L"A<typename T, int> <5:1 <6:7 6:7> 8:1>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitPartialVariableTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename Q>\n"
      "T t = Q(5);\n"
      "\n"
      "template <typename R>\n"
      "int t<int, R> = 9;\n");

  EXPECT_THAT(client->globalVariables, testing::Contains(L"int t<int, typename R> <5:5 5:5>"));
}

TEST_F(CxxParserTestSuite, FindsCorrectFieldMemberNameOfTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	int foo;\n"
      "};\n");

  EXPECT_THAT(client->fields, testing::Contains(L"private int A<typename T>::foo <4:6 4:8>"));
}

TEST_F(CxxParserTestSuite, FindsCorrectTypeOfFieldMemberOfTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T foo;\n"
      "};\n"
      "A<int> a; \n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"int A<int>::foo -> int <4:2 4:2>"));

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> a -> A<int> <6:1 6:1>"));

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> a -> int <6:3 6:5>"));

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <4:2 4:2>"));
}

TEST_F(CxxParserTestSuite, FindsCorrectMethodMemberNameOfTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	int foo();\n"
      "};\n");

  EXPECT_THAT(client->methods, testing::Contains(L"private int A<typename T>::foo() <4:2 <4:6 4:8> 4:10>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(T a)\n"
      "{\n"
      "	return a;\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <1:20 1:20>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeIntTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"
      "int test(int a)\n"
      "{\n"
      "	return a + T;\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:15> <1:15 1:15>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeBoolTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T>\n"
      "int test(int a)\n"
      "{\n"
      "	return T ? a : 0;\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:16> <1:16 1:16>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeCustomPointerTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P* p>\n"
      "int test(int a)\n"
      "{\n"
      "	return a;\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:14> <3:14 3:14>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeCustomReferenceTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class P\n"
      "{};\n"
      "template <P& p>\n"
      "int test(int a)\n"
      "{\n"
      "	return a;\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:14> <3:14 3:14>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateTemplateParameterDefinitionOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T>\n"
      "int test(int a)\n"
      "{\n"
      "	return a;\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:36> <4:36 4:36>"));
}

TEST_F(CxxParserTestSuite, FindsFunctionForImplicitInstantiationOfTemplateFunction) {
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

  EXPECT_THAT(client->functions, testing::Contains(L"int test<int>(int) <2:1 <2:1 <2:3 2:6> 2:11> 5:1>"));
}

TEST_F(CxxParserTestSuite, SkipsImplicitTemplateMethodDefinitionOfImplicitTemplateClassInstantiation) {
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

  EXPECT_THAT(client->methods, testing::Not(testing::Contains(L"public void A<int>::foo<typename U>() <6:2 <6:7 6:9> 6:14>")));
  EXPECT_THAT(client->templateSpecializations,
              testing::Contains(L"void A<int>::foo<float>() -> void A<typename T>::foo<typename U>() <6:7 6:9>"));
}

TEST_F(CxxParserTestSuite, FindsLambdaDefinitionAndCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaCaller()\n"
      "{\n"
      "	[](){}();\n"
      "}\n");

  // TODO: fix
  // TS_ASSERT(utility::containsElement<std::wstring>(
  // 	client->functions, testing::Contains(L"void lambdaCaller::lambda at 3:2() const <3:5 <3:2 3:2> 3:7>"
  // ));
  EXPECT_THAT(client->calls, testing::Contains(L"void lambdaCaller() -> void lambdaCaller::lambda at 3:2() const <3:8 3:8>"));
}

TEST_F(CxxParserTestSuite, FindsMutableLambdaDefinition) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaWrapper()\n"
      "{\n"
      "	[](int foo) mutable { return foo; };\n"
      "}\n");

  // TODO: fix
  // TS_ASSERT(utility::containsElement<std::wstring>(
  // 	client->functions, testing::Contains(L"int lambdaWrapper::lambda at 3:2(int) <3:14 <3:2 3:2> 3:36>"
  // ));
}

TEST_F(CxxParserTestSuite, FindsLocalVariableDeclaredInLambdaCapture) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaWrapper()\n"
      "{\n"
      "	[x(42)]() { return x; };\n"
      "}\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:3> <3:3 3:3>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:3> <3:21 3:21>"));
}

TEST_F(CxxParserTestSuite, FindsDefinitionOfLocalSymbolInFunctionParameterList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void test(int a)\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:15> <1:15 1:15>"));
}

TEST_F(CxxParserTestSuite, FindsDefinitionOfLocalSymbolInFunctionScope) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void test()\n"
      "{\n"
      "	int a;\n"
      "}\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:6> <3:6 3:6>"));
}

///////////////////////////////////////////////////////////////////////////////
// test finding nested symbol definitions and declarations

TEST_F(CxxParserTestSuite, FindsClassDefinitionInClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "public:\n"
      "	class B;\n"
      "};\n");

  EXPECT_THAT(client->classes, testing::Contains(L"public A::B <4:8 4:8>"));
}

TEST_F(CxxParserTestSuite, FindsClassDefinitionInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace a\n"
      "{\n"
      "	class B;\n"
      "};\n");

  EXPECT_THAT(client->classes, testing::Contains(L"a::B <3:8 3:8>"));
}

TEST_F(CxxParserTestSuite, FindsStructDefinitionInClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	struct B\n"
      "	{\n"
      "	};\n"
      "};\n");

  EXPECT_THAT(client->structs, testing::Contains(L"private A::B <3:2 <3:9 3:9> 5:2>"));
}

TEST_F(CxxParserTestSuite, FindsStructDefinitionInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace A\n"
      "{\n"
      "	struct B\n"
      "	{\n"
      "	};\n"
      "};\n");

  EXPECT_THAT(client->structs, testing::Contains(L"A::B <3:2 <3:9 3:9> 5:2>"));
}

TEST_F(CxxParserTestSuite, FindsStructDefinitionInFunction) {
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

  EXPECT_THAT(client->structs, testing::Contains(L"foo::B <3:2 <3:9 3:9> 5:2>"));
  EXPECT_THAT(client->structs, testing::Contains(L"foo::B <9:2 <9:9 9:9> 11:2>"));
}

TEST_F(CxxParserTestSuite, FindsVariableDefinitionsInNamespaceScope) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace n"
      "{\n"
      "	int x;\n"
      "}\n");

  EXPECT_THAT(client->globalVariables, testing::Contains(L"int n::x <2:6 2:6>"));
}

TEST_F(CxxParserTestSuite, FindsFieldInNestedClass) {
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

  EXPECT_THAT(client->fields, testing::Contains(L"private static const int B::C::amount <7:20 7:25>"));
}

TEST_F(CxxParserTestSuite, FindsFunctionInAnonymousNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace\n"
      "{\n"
      "	int sum(int a, int b);\n"
      "}\n");

  EXPECT_THAT(client->functions, testing::Contains(L"int anonymous namespace (temp.cpp<1:1>)::sum(int, int) <3:2 <3:6 3:8> 3:22>"));
}

TEST_F(CxxParserTestSuite, FindsMethodDeclaredInNestedClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "	class C\n"
      "	{\n"
      "		bool isGreat() const;\n"
      "	};\n"
      "};\n");

  EXPECT_THAT(client->methods, testing::Contains(L"private bool B::C::isGreat() const <5:3 <5:8 5:14> 5:22>"));
}

TEST_F(CxxParserTestSuite, FindsNestedNamedNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace A\n"
      "{\n"
      "	namespace B\n"
      "	{\n"
      "	}\n"
      "}\n");

  EXPECT_THAT(client->namespaces, testing::Contains(L"A::B <3:2 <3:12 3:12> 5:2>"));
}

TEST_F(CxxParserTestSuite, FindsEnumDefinedInClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class B\n"
      "{\n"
      "public:\n"
      "	enum Z\n"
      "	{\n"
      "	};\n"
      "};\n");

  EXPECT_THAT(client->enums, testing::Contains(L"public B::Z <4:2 <4:7 4:7> 6:2>"));
}

TEST_F(CxxParserTestSuite, FindsEnumDefinedInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace n\n"
      "{\n"
      "	enum Z\n"
      "	{\n"
      "	};\n"
      "}\n");

  EXPECT_THAT(client->enums, testing::Contains(L"n::Z <3:2 <3:7 3:7> 5:2>"));
}

TEST_F(CxxParserTestSuite, FindsEnumDefinitionInTemplateClass) {
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

  EXPECT_THAT(client->enums, testing::Contains(L"private A<typename T>::TestType <4:2 <4:7 4:14> 8:2>"));
}

TEST_F(CxxParserTestSuite, FindsEnumConstantsInTemplateClass) {
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

  EXPECT_THAT(client->enumConstants, testing::Contains(L"A<typename T>::TestType::TEST_ONE <6:3 6:10>"));
}

///////////////////////////////////////////////////////////////////////////////
// test qualifier locations

TEST_F(CxxParserTestSuite, FindsQualifierOfAccessToGlobalVariableDefinedInNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace foo {\n"
      "	namespace bar {\n"
      "		int x;\n"
      "	}\n"
      "}\n"
      "void f() {\n"
      "	foo::bar::x = 9;\n"
      "}\n");

  EXPECT_THAT(client->qualifiers, testing::Contains(L"foo <7:2 7:4>"));
  EXPECT_THAT(client->qualifiers, testing::Contains(L"foo::bar <7:7 7:9>"));
}

TEST_F(CxxParserTestSuite, FindsQualifierOfAccessToStaticField) {
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

  EXPECT_THAT(client->qualifiers, testing::Contains(L"Foo <9:2 9:4>"));
  EXPECT_THAT(client->qualifiers, testing::Contains(L"Foo::Bar <9:7 9:9>"));
}

TEST_F(CxxParserTestSuite, FindsQualifierOfAccessToEnumConstant) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum Foo {\n"
      "	FOO_V\n"
      "};\n"
      "void f() {\n"
      "	Foo v = Foo::FOO_V;\n"
      "}\n");

  EXPECT_THAT(client->qualifiers, testing::Contains(L"Foo <5:10 5:12>"));
}

TEST_F(CxxParserTestSuite, FindsQualifierOfReferenceToMethod) {
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

  EXPECT_THAT(client->qualifiers, testing::Contains(L"Foo <9:9 9:11>"));
}

TEST_F(CxxParserTestSuite, FindsQualifierOfConstructorCall) {
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

  EXPECT_THAT(client->qualifiers, testing::Contains(L"Foo <8:10 8:12>"));
}

///////////////////////////////////////////////////////////////////////////////
// test implicit symbols

TEST_F(CxxParserTestSuite, FindsBuiltinTypes) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void t1(int v) {}\n"
      "void t2(float v) {}\n"
      "void t3(double v) {}\n"
      "void t4(bool v) {}\n");

  EXPECT_THAT(client->builtinTypes, testing::Contains(L"void"));
  EXPECT_THAT(client->builtinTypes, testing::Contains(L"int"));
  EXPECT_THAT(client->builtinTypes, testing::Contains(L"float"));
  EXPECT_THAT(client->builtinTypes, testing::Contains(L"double"));
  EXPECT_THAT(client->builtinTypes, testing::Contains(L"bool"));
}

TEST_F(CxxParserTestSuite, FindsImplicitCopyConstructor) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class TestClass {}\n"
      "void foo()\n"
      "{\n"
      "	TestClass a;\n"
      "	TestClass b(a);\n"
      "}\n");

  EXPECT_THAT(client->methods, testing::Contains(L"public void TestClass::TestClass() <1:7 <1:7 1:15> 1:15>"));
  EXPECT_THAT(client->methods, testing::Contains(L"public void TestClass::TestClass(const TestClass &) <1:7 <1:7 1:15> 1:15>"));
  EXPECT_THAT(client->methods, testing::Contains(L"public void TestClass::TestClass(TestClass &&) <1:7 <1:7 1:15> 1:15>"));
}

///////////////////////////////////////////////////////////////////////////////
// test finding usages of symbols

TEST_F(CxxParserTestSuite, FindsEnumUsageInTemplateClass) {
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

  EXPECT_THAT(
      client->typeUses, testing::Contains(L"A<typename T>::TestType A<typename T>::foo -> A<typename T>::TestType <9:2 9:9>"));
}

TEST_F(CxxParserTestSuite, FindsCorrectFieldMemberTypeOfNestedTemplateClassInDeclaration) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <7:3 7:3>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int A<int>::B::foo -> int <7:3 7:3>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> a -> A<int> <10:1 10:1>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> -> int <10:3 10:5>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> a -> int <10:3 10:5>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int>::B b -> A<int>::B <11:9 11:9>"));
}

TEST_F(CxxParserTestSuite, FindsTypeUsageOfGlobalVariable) {
  std::shared_ptr<TestStorage> client = parseCode("int x;\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"int x -> int <1:1 1:3>"));
}

TEST_F(CxxParserTestSuite, FindsTypedefsTypeUse) {
  std::shared_ptr<TestStorage> client = parseCode("typedef unsigned int uint;\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"uint -> unsigned int <1:9 1:16>"));
}

TEST_F(CxxParserTestSuite, FindsTypedefThatUsesTypeDefinedInNamedNamespace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace test\n"
      "{\n"
      "	struct TestStruct{};\n"
      "}\n"
      "typedef test::TestStruct globalTestStruct;\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"globalTestStruct -> test::TestStruct <5:15 5:24>"));
}

TEST_F(CxxParserTestSuite, FindsTypeUseOfTypedef) {
  std::shared_ptr<TestStorage> client = parseCode(
      "typedef unsigned int uint;\n"
      "uint number;\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"uint number -> uint <2:1 2:4>"));
}

TEST_F(CxxParserTestSuite, FindsClassDefaultPrivateInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B : A {};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A <2:11 2:11>"));
}

TEST_F(CxxParserTestSuite, FindsClassPublicInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B : public A {};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A <2:18 2:18>"));
}

TEST_F(CxxParserTestSuite, FindsClassProtectedInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B : protected A {};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A <2:21 2:21>"));
}

TEST_F(CxxParserTestSuite, FindsClassPrivateInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B : private A {};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A <2:19 2:19>"));
}

TEST_F(CxxParserTestSuite, FindsClassMultipleInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {};\n"
      "class B {};\n"
      "class C\n"
      "	: public A\n"
      "	, private B\n"
      "{};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"C -> A <4:11 4:11>"));
  EXPECT_THAT(client->inheritances, testing::Contains(L"C -> B <5:12 5:12>"));
}

TEST_F(CxxParserTestSuite, FindsStructDefaultPublicInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B : A {};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A <2:12 2:12>"));
}

TEST_F(CxxParserTestSuite, FindsStructPublicInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B : public A {};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A <2:19 2:19>"));
}

TEST_F(CxxParserTestSuite, FindsStructProtectedInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B : protected A {};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A <2:22 2:22>"));
}

TEST_F(CxxParserTestSuite, FindsStructPrivateInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B : private A {};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A <2:20 2:20>"));
}

TEST_F(CxxParserTestSuite, FindsStructMultipleInheritance) {
  std::shared_ptr<TestStorage> client = parseCode(
      "struct A {};\n"
      "struct B {};\n"
      "struct C\n"
      "	: public A\n"
      "	, private B\n"
      "{};\n");

  EXPECT_THAT(client->inheritances, testing::Contains(L"C -> A <4:11 4:11>"));
  EXPECT_THAT(client->inheritances, testing::Contains(L"C -> B <5:12 5:12>"));
}

TEST_F(CxxParserTestSuite, FindsMethodOverrideWhenVirtual) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	virtual void foo();\n"
      "};\n"
      "class B : public A {\n"
      "	void foo();\n"
      "};");

  EXPECT_THAT(client->overrides, testing::Contains(L"void B::foo() -> void A::foo() <5:7 5:9>"));
}

TEST_F(CxxParserTestSuite, FindsMultiLayerMethodOverrides) {
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

  EXPECT_THAT(client->overrides, testing::Contains(L"void B::foo() -> void A::foo() <5:7 5:9>"));
  EXPECT_THAT(client->overrides, testing::Contains(L"void C::foo() -> void B::foo() <8:7 8:9>"));
}

TEST_F(CxxParserTestSuite, FindsMethodOverridesOnDifferentReturnTypes) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	virtual void foo();\n"
      "};\n"
      "class B : public A {\n"
      "	int foo();\n"
      "};\n");

  EXPECT_EQ(client->errors.size(), 1);
  EXPECT_THAT(client->overrides, testing::Contains(L"int B::foo() -> void A::foo() <5:6 5:8>"));
}

TEST_F(CxxParserTestSuite, FindsNoMethodOverrideWhenNotVirtual) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	void foo();\n"
      "};\n"
      "class B : public A {\n"
      "	void foo();\n"
      "};");

  EXPECT_EQ(client->overrides.size(), 0);
}

TEST_F(CxxParserTestSuite, FindsNoMethodOverridesOnDifferentSignatures) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A {\n"
      "	virtual void foo(int a);\n"
      "};\n"
      "class B : public A {\n"
      "	int foo(int a, int b);\n"
      "};\n");

  EXPECT_EQ(client->overrides.size(), 0);
}

TEST_F(CxxParserTestSuite, FindsUsingDirectiveDeclInFunctionContext) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void foo()\n"
      "{\n"
      "	using namespace std;\n"
      "}\n");

  EXPECT_THAT(client->usages, testing::Contains(L"void foo() -> std <3:18 3:20>"));
}

TEST_F(CxxParserTestSuite, FindsUsingDirectiveDeclInFileContext) {
  std::shared_ptr<TestStorage> client = parseCode("using namespace std;\n");

  EXPECT_THAT(client->usages, testing::Contains(L"temp.cpp -> std <1:17 1:19>"));
}

TEST_F(CxxParserTestSuite, FindsUsingDeclInFunctionContext) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace foo\n"
      "{\n"
      "	int a;\n"
      "}\n"
      "void bar()\n"
      "{\n"
      "	using foo::a;\n"
      "}\n");

  EXPECT_THAT(client->usages, testing::Contains(L"void bar() -> foo::a <7:13 7:13>"));
}

TEST_F(CxxParserTestSuite, FindsUsingDeclInFileContext) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace foo\n"
      "{\n"
      "	int a;\n"
      "}\n"
      "using foo::a;\n");

  EXPECT_THAT(client->usages, testing::Contains(L"temp.cpp -> foo::a <5:12 5:12>"));
}

TEST_F(CxxParserTestSuite, FindsCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int sum(int a, int b)\n"
      "{\n"
      "	return a + b;\n"
      "}\n"
      "int main()\n"
      "{\n"
      "	sum(1, 2);\n"
      "}\n");

  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> int sum(int, int) <7:2 7:4>"));
}

TEST_F(CxxParserTestSuite, FindsCallInFunctionWithCorrectSignature) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"void func(bool) -> int sum(int, int) <10:2 10:4>"));
}

TEST_F(CxxParserTestSuite, FindsCallToFunctionWithRightSignature) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> int sum(int, int) <11:2 11:4>"));
  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> float sum(float, float) <12:2 12:4>"));
}

TEST_F(CxxParserTestSuite, FindsFunctionCallInFunctionParameterList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int sum(int a, int b)\n"
      "{\n"
      "	return a + b;\n"
      "}\n"
      "int main()\n"
      "{\n"
      "	return sum(1, sum(2, 3));\n"
      "}\n");

  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> int sum(int, int) <7:16 7:18>"));
}

TEST_F(CxxParserTestSuite, FindsFunctionCallInMethod) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"int App::main() -> int sum(int, int) <9:10 9:12>"));
}

TEST_F(CxxParserTestSuite, FindsImplicitConstructorWithoutDefinitionCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "};\n"
      "int main()\n"
      "{\n"
      "	App app;\n"
      "}\n");

  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> void App::App() <6:6 6:8>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitConstructorCall) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> void App::App() <8:2 8:4>"));
}

TEST_F(CxxParserTestSuite, FindsCallOfExplicitlyDefinedDestructorAtDeleteKeyword) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"void foo() -> void Foo::~Foo() <11:2 11:7>"));
}

TEST_F(CxxParserTestSuite, FindsCallOfImplicitlyDefinedDestructorAtDeleteKeyword) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"void foo() -> void Foo::~Foo() <8:2 8:7>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitConstructorCallOfField) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"void App::App() -> void Item::Item() <7:10 7:13>"));
}

TEST_F(CxxParserTestSuite, FindsFunctionCallInMemberInitialization) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"void App::App() -> int one() <10:10 10:12>"));
}

TEST_F(CxxParserTestSuite, FindsCopyConstructorCall) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> void App::App(const App &) <10:6 10:9>"));
}

TEST_F(CxxParserTestSuite, FindsGlobalVariableConstructorCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "public:\n"
      "	App() {}\n"
      "};\n"
      "App app;\n");

  EXPECT_THAT(client->calls, testing::Contains(L"App app -> void App::App() <6:5 6:7>"));
}

TEST_F(CxxParserTestSuite, FindsGlobalVariableFunctionCall) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int one() { return 1; }\n"
      "int a = one();\n");

  EXPECT_THAT(client->calls, testing::Contains(L"int a -> int one() <2:9 2:11>"));
}

TEST_F(CxxParserTestSuite, FindsOperatorCall) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> void App::operator+(int) <11:6 11:6>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfFunctionPointer) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"void test() -> void my_int_func(int) <8:9 8:19>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfGlobalVariableInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int bar;\n"
      "\n"
      "int main()\n"
      "{\n"
      "	bar = 1;\n"
      "}\n");

  EXPECT_THAT(client->usages, testing::Contains(L"int main() -> int bar <5:2 5:4>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfGlobalVariableInGlobalVariableInitialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int a = 0;\n"
      "int b[] = {a};\n");

  EXPECT_THAT(client->usages, testing::Contains(L"int [] b -> int a <2:12 2:12>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfGlobalVariableInMethod) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"void App::foo() -> int bar <7:3 7:5>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfFieldInMethod) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"void App::foo() -> int App::bar <5:3 5:5>"));
  EXPECT_THAT(client->usages, testing::Contains(L"void App::foo() -> int App::bar <6:9 6:11>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfFieldInFunctionCallArguments) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"void A::foo(int) -> int A::bar <6:7 6:9>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfFieldInFunctionCallContext) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"void A::foo(int) -> A * A::a <6:3 6:3>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfFieldInInitializationList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "	App()\n"
      "		: bar(42)\n"
      "	{}\n"
      "	int bar;\n"
      "};\n");

  EXPECT_THAT(client->usages, testing::Contains(L"void App::App() -> int App::bar <4:5 4:7>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfMemberInCallExpressionToUnresolvedMemberExpression) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"T B::run<typename T>() -> A B::a <8:10 8:10>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfMemberInTemporaryObjectExpression) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"void Bar::baba() -> const Foo Bar::m_i <15:7 15:9>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfMemberInDependentScopeMemberExpression) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"void A<typename T>::foo() -> T A<typename T>::m_t <8:3 8:5>"));
}

TEST_F(CxxParserTestSuite, FindsReturnTypeUseInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "double PI()\n"
      "{\n"
      "	return 3.14159265359;\n"
      "}\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"double PI() -> double <1:1 1:6>"));
}

TEST_F(CxxParserTestSuite, FindsParameterTypeUsesInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void ceil(float a)\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"void ceil(float) -> float <1:11 1:15>"));
}

TEST_F(CxxParserTestSuite, FindsUseOfDecayedParameterTypeInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template<class T, unsigned int N>\n"
      "class VectorBase\n"
      "{\n"
      "public:\n"
      "	VectorBase(T values[N]);\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:16> <5:13 5:13>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:32> <5:22 5:22>"));
}

TEST_F(CxxParserTestSuite, UsageOfInjectedTypeInMethodDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class Foo\n"
      "{\n"
      "	Foo& operator=(const Foo&) = delete;\n"
      "};\n");

  EXPECT_THAT(client->typeUses,
              testing::Contains(L"Foo<typename T> & Foo<typename T>::operator=(const Foo<typename T> &) -> Foo<typename T> "
                                L"<4:2 4:4>"));
  EXPECT_THAT(client->typeUses,
              testing::Contains(L"Foo<typename T> & Foo<typename T>::operator=(const Foo<typename T> &) -> Foo<typename T> "
                                L"<4:23 4:25>"));
}

TEST_F(CxxParserTestSuite, FindsUseOfQualifiedTypeInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void test(const int t)\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"void test(const int) -> int <1:17 1:19>"));
}

TEST_F(CxxParserTestSuite, FindsParameterTypeUsesInConstructor) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	A(int a);\n"
      "};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"void A::A(int) -> int <3:4 3:6>"));
}

TEST_F(CxxParserTestSuite, FindsTypeUsesInFunctionBody) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int main()\n"
      "{\n"
      "	int a = 42;\n"
      "}\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <3:2 3:4>"));
}

TEST_F(CxxParserTestSuite, FindsTypeUsesInMethodBody) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	int main()\n"
      "	{\n"
      "		int a = 42;\n"
      "		return a;\n"
      "	}\n"
      "};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"int A::main() -> int <5:3 5:5>"));
}

TEST_F(CxxParserTestSuite, FindsTypeUsesInLoopsAndConditions) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <5:3 5:5>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <7:7 7:9>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <9:3 9:5>"));
}

TEST_F(CxxParserTestSuite, FindsTypeUsesOfBaseClassInDerivedConstructor) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"void B::B() -> A <9:8 9:8>"));
}

TEST_F(CxxParserTestSuite, FindsEnumUsesInGlobalSpace) {
  std::shared_ptr<TestStorage> client = parseCode(
      "enum A\n"
      "{\n"
      "	B,\n"
      "	C\n"
      "};\n"
      "A a = B;\n"
      "A* aPtr = new A;\n");

  EXPECT_THAT(client->usages, testing::Contains(L"A a -> A::B <6:7 6:7>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A a -> A <6:1 6:1>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A * aPtr -> A <7:1 7:1>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A * aPtr -> A <7:15 7:15>"));
}

TEST_F(CxxParserTestSuite, FindsEnumUsesInFunctionBody) {
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

  EXPECT_THAT(client->usages, testing::Contains(L"int main() -> A::B <8:8 8:8>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> A <8:2 8:2>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> A <9:2 9:2>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> A <9:16 9:16>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfTemplateParameterOfTemplateMemberVariableDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "struct IsBaseType {\n"
      "	static const bool value = true;\n"
      "};\n"
      "template <typename T>\n"
      "const bool IsBaseType<T>::value;\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <1:20 1:20>"));
  EXPECT_THAT(    // TODO: fix FAIL because usage in name
                  // qualifier is not recorded
      client->localSymbols,
      testing::Contains(L"temp.cpp<5:20> <5:20 5:20>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfTemplateParametersWithDifferentDepthOfTemplateFunction) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <7:3 7:3>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:21> <5:11 5:11>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfTemplateParametersWithDifferentDepthOfPartialClassTemplateSpecialization) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <13:3 13:3>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<10:21> <13:9 13:9>"));
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:36> <7:11 7:11>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfTemplateTemplateParameterOfTemplateClassExplicitlyInstantiatedWithTemplateType) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:36> <8:11 8:11>"));
}

TEST_F(CxxParserTestSuite, FindsTypedefInOtherClassThatDependsOnOwnTemplateParameter) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int>::type -> int <5:10 5:10>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"B<typename U>::type -> A<typename T>::type <11:25 11:28>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"B<int>::type -> A<int>::type <11:25 11:28>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"B<int>::type f -> B<int>::type <13:9 13:12>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfTemplateParameterInQualifierOfOtherSymbol) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:20> <8:49 8:49>"));
}

TEST_F(CxxParserTestSuite, FindsUseOfDependentTemplateSpecializationType) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"B<typename U>::type -> A<typename T>::type<float> <12:10 12:17>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"B<bool>::type f -> B<bool>::type <14:10 14:13>"));
}

TEST_F(CxxParserTestSuite, CreatesSingleNodeForAllPossibleParameterPackExpansionsOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template<typename T>\n"
      "T adder(T v) { return v; }\n"
      "\n"
      "template<typename T, typename... Args>\n"
      "T adder(T first, Args... args) { return first + adder(args...); }\n"
      "\n"
      "void foo() { long sum = adder(1, 2, 3, 8, 7); }\n");

  EXPECT_THAT(client->functions, testing::Contains(L"int adder<int, <...>>(int, ...) <5:1 <5:1 <5:3 5:7> 5:30> 5:65>"));
  EXPECT_THAT(client->calls, testing::Contains(L"int adder<int, <...>>(int, ...) -> int adder<int, <...>>(int, ...) <5:49 5:53>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentOfExplicitTemplateInstantiation) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> -> int <7:4 7:6>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <7:4 7:6>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentOfExplicitTemplateInstantiatedWithFunctionPrototype) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "void foo()\n"
      "{\n"
      "	A<int()> a;\n"
      "}\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int()> -> int <7:4 7:6>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"void foo() -> int <7:4 7:6>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"void foo() -> int <7:4 7:6>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentForParameterPackOfExplicitTemplateInstantiation) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename... T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "int main()\n"
      "{\n"
      "   A<int, float>();\n"
      "}\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<<...>> -> int <7:6 7:8>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A<<...>> -> float <7:11 7:15>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <7:6 7:8>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> float <7:11 7:15>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <7:6 7:8>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> float <7:11 7:15>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentInNonDefaultConstructorOfExplicitTemplateInstantiation) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> -> int <9:4 9:6>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <9:4 9:6>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <9:4 9:6>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentInDefaultConstructorOfExplicitTemplateInstantiation) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> -> int <9:4 9:6>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <9:4 9:6>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <9:4 9:6>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentInNewExpressionOfExplicitTemplateInstantiation) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> -> int <9:8 9:10>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <9:8 9:10>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <9:8 9:10>"));
}

TEST_F(CxxParserTestSuite, FindsNoTemplateArgumentForBuiltinNonTypeIntTemplateParameterOfExplicitTemplateInstantiation) {
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

  EXPECT_EQ(client->typeUses.size(), 3);
}

TEST_F(CxxParserTestSuite, FindsNoTemplateArgumentForBuiltinNonTypeBoolTemplateParameterOfExplicitTemplateInstantiation) {
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

  EXPECT_EQ(client->typeUses.size(), 3);
}

TEST_F(CxxParserTestSuite, FindsNonTypeCustomPointerTemplateArgumentOfImplicitTemplateInstantiation) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<&g_p> -> P g_p <9:5 9:7>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeCustomReferenceTemplateArgumentOfImplicitTemplateInstantiation) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<g_p> -> P g_p <9:4 9:6>"));
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

  EXPECT_EQ(client->typeUses.size(), 3);
}

TEST_F(CxxParserTestSuite, FindsTemplateTemplateArgumentOfExplicitTemplateInstantiation) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"B<A> -> A<typename T> <9:4 9:4>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> A<typename T> <9:4 9:4>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateTemplateArgumentForParameterPackOfExplicitTemplateInstantiation) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"B<<...>> -> A<typename T> <11:4 11:4>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"B<<...>> -> A<typename T> <11:7 11:7>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> A<typename T> <11:4 11:4>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> A<typename T> <11:7 11:7>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateArgumentForImplicitSpecializationOfGlobalTemplateVariable) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T v;\n"
      "void test()\n"
      "{\n"
      "	v<int> = 9;\n"
      "}\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"int v<int> -> int <5:4 5:6>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"void test() -> int <5:4 5:6>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateMemberSpecializationForMethodOfImplicitTemplateSpecialization) {
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

  EXPECT_THAT(client->templateSpecializations, testing::Contains(L"int A<int>::foo() -> T A<typename T>::foo() <5:4 5:6>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateMemberSpecializationForStaticVariableOfImplicitTemplateSpecialization) {
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

  EXPECT_THAT(
      client->templateSpecializations, testing::Contains(L"static int A<int>::foo -> static T A<typename T>::foo <5:11 5:13>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateMemberSpecializationForFieldOfImplicitTemplateSpecialization) {
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

  EXPECT_THAT(client->templateSpecializations, testing::Contains(L"int A<int>::foo -> T A<typename T>::foo <5:4 5:6>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateMemberSpecializationForFieldOfMemberClassOfImplicitTemplateSpecialization) {
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

  EXPECT_THAT(client->templateSpecializations, testing::Contains(L"int A<int>::B::foo -> T A<typename T>::B::foo <7:5 7:7>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateMemberSpecializationForMemberClassOfImplicitTemplateSpecialization) {
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

  EXPECT_THAT(client->templateSpecializations, testing::Contains(L"A<int>::B -> A<typename T>::B <5:8 5:8>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <>\n"
      "class A<int>\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<int> -> int <6:9 6:11>"));
}

TEST_F(CxxParserTestSuite, FindsNoTemplateArgumentForBuiltinNonTypeIntTemplateParameterOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"    // use of "int"
      "class A\n"
      "{\n"
      "};\n"
      "template <>\n"
      "class A<1>\n"
      "{\n"
      "};\n");

  EXPECT_EQ(client->typeUses.size(), 1);
}

TEST_F(CxxParserTestSuite, FindsNoTemplateArgumentForBuiltinNonTypeBoolTemplateParameterOfExplicitTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T>\n"    // use of "bool"
      "class A\n"
      "{\n"
      "};\n"
      "template <>\n"
      "class A<true>\n"
      "{\n"
      "};\n");

  EXPECT_EQ(client->typeUses.size(), 1);
}

TEST_F(CxxParserTestSuite, FindsNonTypeCustomPointerTemplateArgumentOfExplicitTemplateSpecialization) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<&g_p> -> P g_p <8:10 8:12>"));
}

TEST_F(CxxParserTestSuite, FindsNonTypeCustomReferenceTemplateArgumentOfExplicitTemplateSpecialization) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<g_p> -> P g_p <8:9 8:11>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateTemplateArgumentOfExplicitTemplateSpecialization) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"B<A> -> A<typename T> <8:9 8:9>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentsOfExplicitPartialClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T, typename U>\n"
      "class A\n"
      "{\n"
      "};\n"
      "template <typename T>\n"
      "class A<T, int>\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<5:20> <6:9 6:9>"));
  EXPECT_THAT(client->typeUses, testing::Contains(L"A<typename T, int> -> int <6:12 6:14>"));
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<5:15> <6:12 6:12>"));
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<5:16> <6:15 6:15>"));
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

  EXPECT_THAT(client->typeUses,
              testing::Contains(L"A<&g_p, q> -> P g_p <8:10 8:12>"    // TODO: this is completely wrong?
                                                                      // should be a normal usage
                                ));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<7:14> <8:15 8:15>"));
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<g_p, q> -> P g_p <8:9 8:11>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<7:14> <8:14 8:14>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateArgumentForTemplateTemplateParameterOfExplicitPartialClassTemplateSpecialization) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"B<A, template<typename> typename U> -> A<typename T> <8:9 8:9>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<7:36> <8:12 8:12>"));
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<5:27> <6:16 6:17>"));
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<5:36> <6:12 6:13>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<5:48> <6:16 6:17>"));
}

TEST_F(CxxParserTestSuite, FindsImplicitTemplateClassSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T foo;\n"
      "};\n"
      "\n"
      "A<int> a;\n");

  EXPECT_THAT(client->templateSpecializations, testing::Contains(L"A<int> -> A<typename T> <2:7 2:7>"));
}

TEST_F(CxxParserTestSuite, FindsClassInheritanceFromImplicitTemplateClassSpecialization) {
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

  EXPECT_THAT(client->inheritances, testing::Contains(L"B -> A<int> <7:17 7:17>"));
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

TEST_F(CxxParserTestSuite, FindsTemplateClassSpecializationWithTemplateArgument) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<7:20> <8:19 8:19>"));
  EXPECT_EQ(client->inheritances.size(), 1);
  EXPECT_EQ(client->classes.size(), 2);
  EXPECT_EQ(client->fields.size(), 1);
}

TEST_F(CxxParserTestSuite, FindsCorrectOrderOfTemplateArgumentsForExplicitClassTemplateSpecialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T1, typename T2, typename T3>\n"
      "class vector { };\n"
      "template<class Foo1, class Foo2>\n"
      "class vector<Foo2, Foo1, int> { };\n");

  EXPECT_THAT(client->classes, testing::Contains(L"vector<class Foo2, class Foo1, int> <3:1 <4:7 4:12> 4:33>"));
}

TEST_F(CxxParserTestSuite, ReplacesDependentTemplateArgumentsOfExplicitTemplateSpecializationWithNameOfBaseTemplate) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A {};\n"
      "template <typename T1, typename T2>\n"
      "class vector { };\n"
      "template<class Foo1>\n"
      "class vector<Foo1, A<Foo1>> { };\n");

  EXPECT_THAT(client->classes, testing::Contains(L"vector<class Foo1, A<typename T>> <5:1 <6:7 6:12> 6:31>"));
}

TEST_F(CxxParserTestSuite, ReplacesUnknownTemplateArgumentsOfExplicitTemplateSpecializationWithDepthAndPositionIndex) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T0>\n"
      "class foo {\n"
      "	template <typename T1, typename T2>\n"
      "	class vector { };\n"
      "	template<class T1>\n"
      "	class vector<T0, T1> { };\n"
      "};\n");

  EXPECT_THAT(client->classes, testing::Contains(L"foo<typename T0>::vector<arg0_0, class T1> <5:2 <6:8 6:13> 6:25>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateClassConstructorUsageOfField) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	A(): foo() {}\n"
      "	T foo;\n"
      "};\n");

  EXPECT_THAT(client->usages, testing::Contains(L"void A<typename T>::A<T>() -> T A<typename T>::foo <4:7 4:9>"));
}

TEST_F(CxxParserTestSuite, FindsCorrectMethodReturnTypeOfTemplateClassInDeclaration) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{\n"
      "	T foo();\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<1:20> <4:2 4:2>"));

  EXPECT_THAT(client->methods, testing::Contains(L"private T A<typename T>::foo() <4:2 <4:4 4:6> 4:8>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateDefaultArgumentTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T = int>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"A<typename T> -> int <1:24 1:26>"));
}

TEST_F(CxxParserTestSuite, FindsNoDefaultArgumentTypeForNonTypeBoolTemplateParameterOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T = true>\n"
      "class A\n"
      "{\n"
      "};\n");

  EXPECT_EQ(client->typeUses.size(), 1);
  ;    // only the "bool" type is recorded and nothing for the default arg
}

TEST_F(CxxParserTestSuite, FindsTemplateTemplateDefaultArgumentTypeOfTemplateClass) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T = A>\n"
      "class B\n"
      "{};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"B<template<typename> typename T> -> A<typename T> <4:40 4:40>"));
}

TEST_F(CxxParserTestSuite, FindsImplicitInstantiationOfTemplateFunction) {
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

  EXPECT_THAT(client->templateSpecializations, testing::Contains(L"int test<int>(int) -> T test<typename T>(T) <2:3 2:6>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitSpecializationOfTemplateFunction) {
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

  EXPECT_THAT(client->templateSpecializations, testing::Contains(L"int test<int>(int) -> T test<typename T>(T) <8:5 8:8>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitTypeTemplateArgumentOfExplicitInstantiationOfTemplateFunction) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"void test<int>() -> int <7:11 7:13>"));
}

TEST_F(CxxParserTestSuite, FindsExplicitTypeTemplateArgumentOfFunctionCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "void test(){}\n"
      "\n"
      "int main()\n"
      "{\n"
      "	test<int>();\n"
      "	return 1;\n"
      "};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"void test<int>() -> int <6:7 6:9>"));
}

TEST_F(CxxParserTestSuite, FindsNoExplicitNonTypeIntTemplateArgumentOfFunctionCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <int T>\n"    // use of "int"
      "void test(){}\n"       // 2x use of "void"
      "\n"
      "int main()\n"    // use of "int"
      "{\n"
      "	test<33>();\n"
      "	return 1;\n"
      "};\n");

  EXPECT_EQ(client->typeUses.size(), 4);
}

TEST_F(CxxParserTestSuite, FindsExplicitTemplateTemplateArgumentOfFunctionCallInFunction) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"void test<A>() -> A<typename T> <7:7 7:7>"));
}

TEST_F(CxxParserTestSuite, FindsNoImplicitTypeTemplateArgumentOfFunctionCallInFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "void test(T data){}\n"    // 2x use of "void" + 1x use of "int"
      "\n"
      "int main()\n"    // use of "int"
      "{\n"
      "	test(1);\n"
      "	return 1;\n"
      "};\n");

  EXPECT_EQ(client->typeUses.size(), 4);
}

TEST_F(CxxParserTestSuite, FindsExplicitTypeTemplateArgumentOfFunctionCallInVarDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(){ return 1; }\n"
      "\n"
      "class A\n"
      "{\n"
      "	int foo = test<int>();\n"
      "};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"int test<int>() -> int <6:17 6:19>"));
}

TEST_F(CxxParserTestSuite, FindsNoImplicitTypeTemplateArgumentOfFunctionCallInVarDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "T test(T i){ return i; }\n"    // 2x use of "int"
      "\n"
      "class A\n"
      "{\n"
      "	int foo = test(1);\n"    // usage of "int"
      "};\n");

  EXPECT_EQ(client->typeUses.size(), 3);
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateDefaultArgumentTypeOfTemplateFunction) {
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

  EXPECT_THAT(client->typeUses, testing::Contains(L"void test<typename T>() -> int <1:24 1:26>"));
}

TEST_F(CxxParserTestSuite, DoesNotFindDefaultArgumentTypeForNonTypeBoolTemplateParameterOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <bool T = true>\n"
      "void test()\n"
      "{\n"
      "};\n");

  EXPECT_EQ(client->typeUses.size(), 2);
  ;    // only "bool" and "void" is recorded
}

TEST_F(CxxParserTestSuite, FindsTemplateTemplateDefaultArgumentTypeOfTemplateFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "template <typename T>\n"
      "class A\n"
      "{};\n"
      "template <template<typename> class T = A>\n"
      "void test()\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"void test<template<typename> typename T>() -> A<typename T> <4:40 4:40>"));
}

TEST_F(CxxParserTestSuite, FindsLambdaCallingAFunction) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void func() {}\n"
      "void lambdaCaller()\n"
      "{\n"
      "	[]()\n"
      "	{\n"
      "		func();\n"
      "	}();\n"
      "}\n");

  EXPECT_THAT(client->calls, testing::Contains(L"void lambdaCaller::lambda at 4:2() const -> void func() <6:3 6:6>"));
}

TEST_F(CxxParserTestSuite, FindsLocalVariableInLambdaCapture) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaWrapper()\n"
      "{\n"
      "	int x = 2;\n"
      "	[x]() { return 1; }();\n"
      "}\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:6> <4:3 4:3>"));
}

TEST_F(CxxParserTestSuite, FindsUsageOfLocalVariableInMicrosoftInlineAssemblyStatement) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:6> <6:11 6:11>"));

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:6> <7:6 7:6>"));
}

TEST_F(CxxParserTestSuite, FindsTemplateArgumentOfUnresolvedLookupExpression) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<6:20> <9:4 9:14>"));
}

///////////////////////////////////////////////////////////////////////////////
// test finding symbol locations

TEST_F(CxxParserTestSuite, FindsCorrectLocationOfExplicitConstructorDefinedInNamespace) {
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

  EXPECT_THAT(client->calls, testing::Contains(L"int main() -> void n::App::App(int) <11:16 11:18>"));
}

TEST_F(CxxParserTestSuite, FindsMacroArgumentLocationForFieldDefinitionWithNamePassedAsArgumentToMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define DEF_INT_FIELD(name) int name;\n"
      "class A {\n"
      "	DEF_INT_FIELD(m_value)\n"
      "};\n");

  EXPECT_THAT(client->fields, testing::Contains(L"private int A::m_value <3:16 3:22>"));
}

TEST_F(CxxParserTestSuite, FindsMacroUsageLocationForFieldDefinitionWithNamePartiallyPassedAsArgumentToMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define DEF_INT_FIELD(name) int m_##name;\n"
      "class A {\n"
      "	DEF_INT_FIELD(value)\n"
      "};\n");

  EXPECT_THAT(client->fields, testing::Contains(L"private int A::m_value <3:2 3:14>"));
}

TEST_F(CxxParserTestSuite, FindsMacroArgumentLocationForFunctionCallInCodePassedAsArgumentToMacro) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define DEF_INT_FIELD(name, init) int name = init;\n"
      "int foo() { return 5; }\n"
      "class A {\n"
      "	DEF_INT_FIELD(m_value, foo())\n"
      "};\n");

  EXPECT_THAT(client->calls, testing::Contains(L"int A::m_value -> int foo() <4:25 4:27>"));
}

TEST_F(CxxParserTestSuite, FindsMacroUsageLocationForFunctionCallInCodeOfMacroBody) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int foo() { return 5; }\n"
      "#define DEF_INT_FIELD(name) int name = foo();\n"
      "class A {\n"
      "	DEF_INT_FIELD(m_value)\n"
      "};\n");

  EXPECT_THAT(client->calls, testing::Contains(L"int A::m_value -> int foo() <4:2 4:14>"));
}

TEST_F(CxxParserTestSuite, FindsTypeTemplateArgumentOfStaticCastExpression) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int main()\n"
      "{\n"
      "	return static_cast<int>(4.0f);"
      "}\n");

  EXPECT_THAT(client->typeUses, testing::Contains(L"int main() -> int <3:21 3:23>"));
}

TEST_F(CxxParserTestSuite, FindsImplicitConstructorCallInInitialization) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "};\n"
      "class B\n"
      "{\n"
      "	B(){}\n"
      "	A m_a;\n"
      "};\n");

  EXPECT_THAT(client->calls, testing::Contains(L"void B::B() -> void A::A() <6:2 6:2>"));
}

TEST_F(CxxParserTestSuite, ParsesMultipleFiles) {
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
      std::vector<std::wstring>{L"--target=x86_64-pc-windows-msvc", L"-std=c++17", sourceFilePath.wstr()});

  std::shared_ptr<IntermediateStorage> storage = std::make_shared<IntermediateStorage>();
  CxxParser parser(std::make_shared<ParserClientImpl>(storage.get()),
                   std::make_shared<TestFileRegister>(),
                   std::make_shared<IndexerStateInfo>());

  parser.buildIndex(indexerCommand);

  std::shared_ptr<TestStorage> testStorage = TestStorage::create(storage);

  EXPECT_EQ(testStorage->errors.size(), 0);

  EXPECT_EQ(testStorage->typedefs.size(), 1);
  EXPECT_EQ(testStorage->classes.size(), 4);
  EXPECT_EQ(testStorage->enums.size(), 1);
  EXPECT_EQ(testStorage->enumConstants.size(), 2);
  EXPECT_EQ(testStorage->functions.size(), 2);
  EXPECT_EQ(testStorage->fields.size(), 4);
  EXPECT_EQ(testStorage->globalVariables.size(), 2);
  EXPECT_EQ(testStorage->methods.size(), 15);
  EXPECT_EQ(testStorage->namespaces.size(), 2);
  EXPECT_EQ(testStorage->structs.size(), 1);

  EXPECT_EQ(testStorage->inheritances.size(), 1);
  EXPECT_EQ(testStorage->calls.size(), 3);
  EXPECT_EQ(testStorage->usages.size(), 3);
  EXPECT_EQ(testStorage->typeUses.size(), 16);

  EXPECT_EQ(testStorage->files.size(), 2);
  EXPECT_EQ(testStorage->includes.size(), 1);
}

TEST_F(CxxParserTestSuite, FindsBracesOfClassDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:1> <2:1 2:1>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:1> <3:1 3:1>"));
}

TEST_F(CxxParserTestSuite, FindsBracesOfNamespaceDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "namespace n\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:1> <2:1 2:1>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:1> <3:1 3:1>"));
}

TEST_F(CxxParserTestSuite, FindsBracesOfFunctionDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int main()\n"
      "{\n"
      "}\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:1> <2:1 2:1>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:1> <3:1 3:1>"));
}

TEST_F(CxxParserTestSuite, FindsBracesOfMethodDecl) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class App\n"
      "{\n"
      "public:\n"
      "	App(int i) {}\n"
      "};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:13> <4:13 4:13>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:13> <4:14 4:14>"));
}

TEST_F(CxxParserTestSuite, FindsBracesOfInitList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "int a = 0;\n"
      "int b[] = {a};\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:11> <2:11 2:11>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<2:11> <2:13 2:13>"));
}

TEST_F(CxxParserTestSuite, FindsBracesOfLambda) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void lambdaCaller()\n"
      "{\n"
      "	[](){}();\n"
      "}\n");

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:6> <3:6 3:6>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:6> <3:7 3:7>"));
}

TEST_F(CxxParserTestSuite, FindsBracesOfAsmStmt) {
  std::shared_ptr<TestStorage> client = parseCode(
      "void foo()\n"
      "{\n"
      "	__asm\n"
      "	{\n"
      "		mov eax, eax\n"
      "	}\n"
      "}\n",
      {L"--target=i686-pc-windows-msvc"});

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:2> <4:2 4:2>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<4:2> <6:2 6:2>"));
}

TEST_F(CxxParserTestSuite, FindsNoDuplicateBracesOfTemplateClassAndMethodDecl) {
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

  EXPECT_EQ(client->localSymbols.size(), 9);
  ;    // 8 braces + 1 template parameter
}

TEST_F(CxxParserTestSuite, FindsBracesWithClosingBracketInMacro) {
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:1> <3:1 3:1>"));
  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<3:1> <7:2 7:2>"));
  // TS_ASSERT(utility::containsElement<std::wstring>(client->localSymbols, testing::Contains(L"<0:0> <11:1
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

  EXPECT_THAT(client->localSymbols, testing::Contains(L"temp.cpp<7:1> <7:1 7:1>"));
  // TS_ASSERT(utility::containsElement<std::wstring>(client->localSymbols, testing::Contains(L"temp.cpp<7:1> <10:1
  // 10:1>")); // missing TS_ASSERT(utility::containsElement<std::wstring>(client->localSymbols,
  // L"<0:0> <10:1 10:1>")); // unwanted sideeffect
}

TEST_F(CxxParserTestSuite, FindsCorrectSignatureLocationOfConstructorWithInitializerList) {
  std::shared_ptr<TestStorage> client = parseCode(
      "class A\n"
      "{\n"
      "	A(const int& foo) : m_foo(foo)\n"
      "	{\n"
      "	}\n"
      "	const int m_foo\n"
      "}\n");
  ;

  EXPECT_THAT(client->methods, testing::Contains(L"private void A::A(const int &) <3:2 <3:2 <3:2 3:2> 3:18> 5:2>"));
}

TEST_F(CxxParserTestSuite, CatchesError) {
  std::shared_ptr<TestStorage> client = parseCode("int a = b;\n");

  EXPECT_THAT(client->errors, testing::Contains(L"use of undeclared identifier \'b\' <1:9 1:9>"));
}

TEST_F(CxxParserTestSuite, CatchesErrorInForceInclude) {
  std::shared_ptr<TestStorage> client = parseCode("void foo() {} \n", {L"-include nothing"});

  EXPECT_THAT(client->errors, testing::Contains(L"' nothing' file not found <1:1 1:1>"));
}

TEST_F(CxxParserTestSuite, FindsCorrectErrorLocationAfterLineDirective) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#line 55 \"foo.hpp\"\n"
      "void foo()\n");

  EXPECT_THAT(client->errors, testing::Contains(L"expected function body after function declarator <2:11 2:11>"));
}

TEST_F(CxxParserTestSuite, CatchesErrorInMacroExpansion) {
  std::shared_ptr<TestStorage> client = parseCode(
      "#define MACRO_WITH_NONEXISTENT_PATH \"this_path_does_not_exist.txt\"\n"
      "#include MACRO_WITH_NONEXISTENT_PATH\n");

  EXPECT_THAT(client->errors, testing::Contains(L"'this_path_does_not_exist.txt' file not found <2:10 2:10>"));
}

TEST_F(CxxParserTestSuite, FindsLocationOfLineComment) {
  std::shared_ptr<TestStorage> client = parseCode("// this is a line comment\n");

  EXPECT_THAT(client->comments, testing::Contains(L"comment <1:1 1:26>"));
}

TEST_F(CxxParserTestSuite, FindsLocationOfBlockComment) {
  std::shared_ptr<TestStorage> client = parseCode(
      "/* this is a\n"
      "block comment */\n");

  EXPECT_THAT(client->comments, testing::Contains(L"comment <1:1 2:17>"));
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