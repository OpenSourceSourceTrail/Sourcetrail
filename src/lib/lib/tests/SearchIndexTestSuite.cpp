#include <gtest/gtest.h>

#include "NameHierarchy.h"
#include "SearchIndex.h"
#include "utility.h"

TEST(SearchIndex, searchIndexFindsIdOfElementAdded) {
  SearchIndex index;
  index.addNode(1, NameHierarchy::deserialize(L"::\tmfoo\tsvoid\tp() const").getQualifiedName());
  index.finishSetup();
  std::vector<SearchResult> results = index.search(L"oo", NodeTypeSet::all(), 0);

  EXPECT_TRUE(1 == results.size());
  EXPECT_TRUE(1 == results[0].elementIds.size());
  EXPECT_TRUE(utility::containsElement<Id>(results[0].elementIds, 1));
}

TEST(SearchIndex, searchIndexFindsCorrectIndicesForQuery) {
  SearchIndex index;
  index.addNode(1, NameHierarchy::deserialize(L"::\tmfoo\tsvoid\tp() const").getQualifiedName());
  index.finishSetup();
  std::vector<SearchResult> results = index.search(L"oo", NodeTypeSet::all(), 0);

  EXPECT_TRUE(1 == results.size());
  EXPECT_TRUE(2 == results[0].indices.size());
  EXPECT_TRUE(1 == results[0].indices[0]);
  EXPECT_TRUE(2 == results[0].indices[1]);
}

TEST(SearchIndex, searchIndexFindsIdsForAmbiguousQuery) {
  SearchIndex index;
  index.addNode(1, NameHierarchy::deserialize(L"::\tmfor\tsvoid\tp() const").getQualifiedName());
  index.addNode(2, NameHierarchy::deserialize(L"::\tmfos\tsvoid\tp() const").getQualifiedName());
  index.finishSetup();
  std::vector<SearchResult> results = index.search(L"fo", NodeTypeSet::all(), 0);

  EXPECT_TRUE(2 == results.size());
  EXPECT_TRUE(1 == results[0].elementIds.size());
  EXPECT_TRUE(utility::containsElement<Id>(results[0].elementIds, 1));
  EXPECT_TRUE(1 == results[1].elementIds.size());
  EXPECT_TRUE(utility::containsElement<Id>(results[1].elementIds, 2));
}

TEST(SearchIndex, searchIndexDoesNotFindAnythingAfterClear) {
  SearchIndex index;
  index.addNode(1, NameHierarchy::deserialize(L"::\tmfoo\tsvoid\tp() const").getQualifiedName());
  index.finishSetup();
  index.clear();
  std::vector<SearchResult> results = index.search(L"oo", NodeTypeSet::all(), 0);

  EXPECT_TRUE(0 == results.size());
}

TEST(SearchIndex, searchIndexDoesNotFindAllResultsWhenMaxAmountIsLimited) {
  SearchIndex index;
  index.addNode(1, NameHierarchy::deserialize(L"::\tmfoo1\tsvoid\tp() const").getQualifiedName());
  index.addNode(2, NameHierarchy::deserialize(L"::\tmfoo2\tsvoid\tp() const").getQualifiedName());
  index.finishSetup();
  std::vector<SearchResult> results = index.search(L"oo", NodeTypeSet::all(), 1);

  EXPECT_TRUE(1 == results.size());
}

TEST(SearchIndex, searchIndexQueryIsCaseInsensitive) {
  SearchIndex index;
  index.addNode(1, NameHierarchy::deserialize(L"::\tmfoo1\tsvoid\tp() const").getQualifiedName());
  index.addNode(2, NameHierarchy::deserialize(L"::\tmFOO2\tsvoid\tp() const").getQualifiedName());
  index.finishSetup();
  std::vector<SearchResult> results = index.search(L"oo", NodeTypeSet::all(), 0);

  EXPECT_TRUE(2 == results.size());
}

TEST(SearchIndex, searchIndexRatesHigherOnConsecutiveLetters) {
  SearchIndex index;
  index.addNode(1, NameHierarchy::deserialize(L"::\tmoaabbcc\tsvoid\tp() const").getQualifiedName());
  index.addNode(2, NameHierarchy::deserialize(L"::\tmocbcabc\tsvoid\tp() const").getQualifiedName());
  index.finishSetup();
  std::vector<SearchResult> results = index.search(L"abc", NodeTypeSet::all(), 0);

  EXPECT_TRUE(2 == results.size());
  EXPECT_TRUE(L"ocbcabc" == results[0].text);
  EXPECT_TRUE(L"oaabbcc" == results[1].text);
}
