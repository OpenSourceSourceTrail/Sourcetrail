#include <gtest/gtest.h>

#include "GlobalId.hpp"
#include "LowMemoryStringMap.h"
#include "TextAccess.h"
#include "utilityString.h"

TEST(LowMemoryStringMap, roughlyEverything) {
  LowMemoryStringMap<std::string, Id, 0> map;

  map.add("abcdefg", 2);
  map.add("abcdefgerlitz", 1);
  map.add("abcdefghij", 3);
  map.add("abc", 4);

  EXPECT_TRUE(map.find("abcdefgerlitz") == 1);
  EXPECT_TRUE(map.find("abcdefg") == 2);
  EXPECT_TRUE(map.find("abcdefghij") == 3);
  EXPECT_TRUE(map.find("abc") == 4);
  EXPECT_TRUE(map.find("bc") == 0);
  EXPECT_TRUE(map.find("") == 0);
  EXPECT_TRUE(map.find(";asdfl;kjasd;flkasdf") == 0);
}

TEST(LowMemoryStringMap, cannotFindElementAfterCreation) {
  LowMemoryStringMap<std::string, Id, 0> map;

  EXPECT_TRUE(map.find("a") == 0);
}

TEST(LowMemoryStringMap, findElement) {
  LowMemoryStringMap<std::string, Id, 0> map;
  map.add("a", 1);

  EXPECT_TRUE(map.find("a") == 1);
  EXPECT_TRUE(map.find("b") == 0);
}

TEST(LowMemoryStringMap, findFullyDifferentElements) {
  LowMemoryStringMap<std::string, Id, 0> map;
  map.add("a", 1);
  map.add("b", 2);

  EXPECT_TRUE(map.find("a") == 1);
  EXPECT_TRUE(map.find("b") == 2);
}

TEST(LowMemoryStringMap, findSimilarShortElements) {
  LowMemoryStringMap<std::string, Id, 0> map;
  map.add("ab", 1);
  map.add("ac", 2);

  EXPECT_TRUE(map.find("ab") == 1);
  EXPECT_TRUE(map.find("ac") == 2);
  EXPECT_TRUE(map.find("bc") == 0);
}

TEST(LowMemoryStringMap, findSimilarLongElements) {
  LowMemoryStringMap<std::string, Id, 0> map;
  map.add("aaaaabbbbb", 1);
  map.add("aaaaaccccc", 2);
  map.add("aaaaccccc", 3);
  map.add("aaaccccc", 4);

  EXPECT_TRUE(map.find("aaaaabbbbb") == 1);
  EXPECT_TRUE(map.find("aaaaaccccc") == 2);
  EXPECT_TRUE(map.find("aaaaacccccc") == 0);
  EXPECT_TRUE(map.find("aacc") == 0);
}

TEST(LowMemoryStringMap, addTwice) {
  LowMemoryStringMap<std::string, Id, 0> map;
  map.add("abba", 1);
  map.add("abba", 2);

  EXPECT_TRUE(map.find("abba") == 1);
}

TEST(LowMemoryStringMap, findParentChildElements) {
  LowMemoryStringMap<std::string, Id, 0> map;
  map.add("a", 1);
  map.add("ab", 2);

  EXPECT_TRUE(map.find("a") == 1);
  EXPECT_TRUE(map.find("ab") == 2);
  EXPECT_TRUE(map.find("b") == 0);
}

TEST(LowMemoryStringMap, findLongParentChildElements) {
  LowMemoryStringMap<std::string, Id, 0> map;
  map.add("ababaaa", 1);
  map.add("aba", 2);

  EXPECT_TRUE(map.find("ababaaa") == 1);
  EXPECT_TRUE(map.find("aba") == 2);
  EXPECT_TRUE(map.find("ab") == 0);
  EXPECT_TRUE(map.find("") == 0);
}

TEST(LowMemoryStringMap, findLongSimilarPrefixElements) {
  LowMemoryStringMap<std::string, Id, 0> map;
  map.add("ababababaab", 1);
  map.add("abababababababaccc", 2);
  map.add("abababababababaer", 3);
  map.add("abababababababber", 4);
  map.add("abababababababaaaa", 5);

  // map.print(std::cout);
  // std::cout << std::endl << map.getByteSize() << " : " << map.getUncompressedByteSize() << std::endl;

  EXPECT_TRUE(map.find("ababababaab") == 1);
  EXPECT_TRUE(map.find("abababababababaccc") == 2);
  EXPECT_TRUE(map.find("abababababababaer") == 3);
  EXPECT_TRUE(map.find("abababababababber") == 4);
  EXPECT_TRUE(map.find("abababababababaaaa") == 5);
  EXPECT_TRUE(map.find("abababab") == 0);
}

TEST(LowMemoryStringMap, wstring) {
  LowMemoryStringMap<std::wstring, Id, 0> map;

  FilePath filePath(L"data/LowMemoryStringMapTestSuite/names.txt");
  std::shared_ptr<TextAccess> textAccess = TextAccess::createFromFile(filePath);

  std::vector<std::wstring> names;
  for(std::string line : textAccess->getAllLines()) {
    names.emplace_back(utility::decodeFromUtf8(line.substr(0, line.find("\n"))));
  }

  for(size_t i = 0; i < names.size(); i++) {
    map.add(names[i], i);
  }

  // map.print(std::wcout);
  // std::cout << std::endl << map.getByteSize() << " : " << map.getUncompressedByteSize() << std::endl;

  for(size_t i = 0; i < names.size(); i++) {
    EXPECT_TRUE(map.find(names[i]) == i);
  }

  // TS_ASSERT(map.getByteSize() < map.getUncompressedByteSize());
}
