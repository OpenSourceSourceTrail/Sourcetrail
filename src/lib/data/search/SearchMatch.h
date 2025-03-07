#pragma once
#include <string>
#include <vector>

#include "GlobalId.hpp"
#include "Node.h"
#include "NodeKind.h"

class NodeTypeSet;

// SearchMatch is used to display the search result in the UI
struct SearchMatch {
  enum SearchType : uint8_t { SEARCH_NONE, SEARCH_TOKEN, SEARCH_COMMAND, SEARCH_OPERATOR, SEARCH_FULLTEXT };

  enum CommandType : uint8_t { COMMAND_ALL, COMMAND_ERROR, COMMAND_NODE_FILTER, COMMAND_LEGEND };

  static void log(const std::vector<SearchMatch>& matches, const std::wstring& query);

  static std::wstring getSearchTypeName(SearchType type);
  static std::wstring searchMatchesToString(const std::vector<SearchMatch>& matches);

  static SearchMatch createCommand(CommandType type);
  static std::vector<SearchMatch> createCommandsForNodeTypes(NodeTypeSet types);
  static std::wstring getCommandName(CommandType type);

  static const wchar_t FULLTEXT_SEARCH_CHARACTER = L'?';

  SearchMatch();
  explicit SearchMatch(const std::wstring& query);

  bool operator<(const SearchMatch& other) const;
  bool operator==(const SearchMatch& other) const;

  static size_t getTextSizeForSorting(const std::wstring* str);

  [[nodiscard]] bool isValid() const;
  [[nodiscard]] bool isFilterCommand() const;

  void print(std::wostream& ostream) const;

  [[nodiscard]] std::wstring getFullName() const;
  [[nodiscard]] std::wstring getSearchTypeName() const;
  [[nodiscard]] CommandType getCommandType() const;

  std::wstring name;

  std::wstring text;
  std::wstring subtext;

  std::vector<Id> tokenIds;
  std::vector<NameHierarchy> tokenNames;

  std::wstring typeName;

  NodeType nodeType = static_cast<NodeType>(NODE_SYMBOL);
  SearchType searchType = SEARCH_NONE;
  std::vector<size_t> indices;

  int score = 0;
  bool hasChildren = false;
};