#include <gtest/gtest.h>

#include "data/parser/cxx/name/CxxTypeName.h"
#include "language_packages.h"

TEST(CxxTypeName, typeNameCreatedWithNameHasNoQualifiersOrModifiers) {
  CxxTypeName typeName(L"int", std::vector<std::wstring>(), std::shared_ptr<CxxName>());
  EXPECT_TRUE(L"int" == typeName.toString());
}

TEST(CxxTypeName, typeNameCreatedWithNameAndConstQualifierHasNoModifiers) {
  CxxTypeName typeName(L"int", std::vector<std::wstring>(), std::shared_ptr<CxxName>());
  typeName.addQualifier(CxxQualifierFlags::QUALIFIER_CONST);
  EXPECT_TRUE(L"const int" == typeName.toString());
}

TEST(CxxTypeName, typeNameCreatedWithNameAndArrayModifierHasArrayModifier) {
  CxxTypeName typeName(L"int", std::vector<std::wstring>(), std::shared_ptr<CxxName>());
  typeName.addModifier(CxxTypeName::Modifier(L"[]"));
  EXPECT_TRUE(L"int []" == typeName.toString());
}

TEST(CxxTypeName, typeNameCreatedWithNameAndConstPointerModifierHasConstPointerModifier) {
  CxxTypeName typeName(L"int", std::vector<std::wstring>(), std::shared_ptr<CxxName>());
  typeName.addModifier(CxxTypeName::Modifier(L"*"));
  typeName.addQualifier(CxxQualifierFlags::QUALIFIER_CONST);
  EXPECT_TRUE(L"int * const" == typeName.toString());
}

TEST(CxxTypeName, typeNameCreatedWithNameAndPointerPointerModifierHasPointerPointerModifier) {
  CxxTypeName typeName(L"int", std::vector<std::wstring>(), std::shared_ptr<CxxName>());
  typeName.addModifier(CxxTypeName::Modifier(L"*"));
  typeName.addModifier(CxxTypeName::Modifier(L"*"));
  EXPECT_TRUE(L"int * *" == typeName.toString());
}