#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Node.h"
#include "NodeTypeSet.h"
#include "GlobalId.hpp"

// SearchResult is only used as an internal type in the SearchIndex and the PersistentStorage
struct SearchResult {
  SearchResult(std::wstring text_, std::vector<Id> elementIds_, std::vector<size_t> indices_, int score_)
      : text(std::move(text_)), elementIds(std::move(elementIds_)), indices(std::move(indices_)), score(score_) {}

  bool operator<(const SearchResult& other) const {
    return score > other.score;
  }

  std::wstring text;
  std::vector<Id> elementIds;
  std::vector<size_t> indices;
  int score;
};

class SearchIndex {
public:
  SearchIndex();
  virtual ~SearchIndex();

  void addNode(Id id, std::wstring name, NodeType type = NodeType(NODE_SYMBOL));
  void finishSetup();
  void clear();

  // maxResultCount == 0 means "no restriction".
  std::vector<SearchResult> search(const std::wstring& query,
                                   NodeTypeSet acceptedNodeTypes,
                                   size_t maxResultCount,
                                   size_t maxBestScoredResultsLength = 0) const;

private:
  struct SearchEdge;

  struct SearchNode {
    SearchNode(NodeTypeSet containedTypes_) : containedTypes(containedTypes_) {}

    std::map<Id, NodeType> elementIds;
    NodeTypeSet containedTypes;
    std::map<wchar_t, SearchEdge*> edges;
  };

  struct SearchEdge {
    SearchEdge(SearchNode* target_, std::wstring text) : target(target_), s(std::move(text)) {}

    SearchNode* target;
    std::wstring s;
    std::set<wchar_t> gate;
  };

  struct SearchPath {
    SearchPath(std::wstring text_, std::vector<size_t> indices_, SearchNode* node_)
        : text(std::move(text_)), indices(std::move(indices_)), node(node_) {}

    std::wstring text;
    std::vector<size_t> indices;
    SearchNode* node;
  };

  void populateEdgeGate(SearchEdge* e);
  void searchRecursive(const SearchPath& path,
                       const std::wstring& remainingQuery,
                       NodeTypeSet acceptedNodeTypes,
                       std::vector<SearchIndex::SearchPath>* results) const;

  std::multiset<SearchResult> createScoredResults(const std::vector<SearchPath>& paths,
                                                  NodeTypeSet acceptedNodeTypes,
                                                  size_t maxResultCount) const;

  static SearchResult bestScoredResult(SearchResult result,
                                       std::map<std::wstring, SearchResult>* scoresCache,
                                       size_t maxBestScoredResultsLength);
  static void bestScoredResultRecursive(const std::wstring& lowerText,
                                        const std::vector<size_t>& indices,
                                        const size_t lastIndex,
                                        const size_t indicesPos,
                                        std::map<std::wstring, SearchResult>* scoresCache,
                                        SearchResult* result);
  static int scoreText(const std::wstring& text, const std::vector<size_t>& indices);

public:
  static SearchResult rescoreText(const std::wstring& fulltext,
                                  const std::wstring& text,
                                  const std::vector<size_t>& indices,
                                  int score,
                                  size_t maxBestScoredResultsLength);

  static bool isNoLetter(const wchar_t c);

private:
  std::vector<std::unique_ptr<SearchNode>> m_nodes;
  std::vector<std::unique_ptr<SearchEdge>> m_edges;
  SearchNode* m_root;
};
