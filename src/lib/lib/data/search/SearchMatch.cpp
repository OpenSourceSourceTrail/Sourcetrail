#include "SearchMatch.h"
// STL
#include <sstream>
// internal
#include "logging.h"
#include "NodeTypeSet.h"

void SearchMatch::log(const std::vector<SearchMatch>& matches, const std::wstring& query) {
  std::wstringstream sStream;
  sStream << '\n' << matches.size() << " matches for \"" << query << "\":" << '\n';

  for(const SearchMatch& match : matches) {
    match.print(sStream);
  }

  LOG_INFO(sStream.str());
}

std::wstring SearchMatch::getSearchTypeName(SearchType type) {
  switch(type) {
  case SEARCH_NONE:
    return L"none";
  case SEARCH_TOKEN:
    return L"token";
  case SEARCH_COMMAND:
    return L"command";
  case SEARCH_OPERATOR:
    return L"operator";
  case SEARCH_FULLTEXT:
    return L"fulltext";
  }

  return L"none";
}

std::wstring SearchMatch::searchMatchesToString(const std::vector<SearchMatch>& matches) {
  std::wstringstream sStream;

  for(const SearchMatch& match : matches) {
    sStream << L'@' << match.getFullName() << L':' << getReadableNodeKindWString(match.nodeType.getKind()) << L' ';
  }

  return sStream.str();
}

SearchMatch SearchMatch::createCommand(CommandType type) {
  SearchMatch match;
  match.name = getCommandName(type);
  match.text = match.name;
  match.typeName = L"command";
  match.searchType = SEARCH_COMMAND;
  return match;
}

std::vector<SearchMatch> SearchMatch::createCommandsForNodeTypes(NodeTypeSet types) {
  std::vector<SearchMatch> matches;

  for(const NodeType& type : types.getNodeTypes()) {
    SearchMatch match;
    match.name = type.getReadableTypeWString();
    match.text = match.name;
    match.typeName = L"filter";
    match.searchType = SEARCH_COMMAND;
    match.nodeType = type;
    matches.push_back(match);
  }

  return matches;
}

std::wstring SearchMatch::getCommandName(CommandType type) {
  switch(type) {
  case COMMAND_ALL:
    return L"overview";
  case COMMAND_ERROR:
    return L"error";
  case COMMAND_NODE_FILTER:
    return L"node_filter";
  case COMMAND_LEGEND:
    return L"legend";
  }

  return L"none";
}

SearchMatch::SearchMatch() = default;

SearchMatch::SearchMatch(const std::wstring& query) : name(query), text(query) {
  tokenNames.emplace_back(query, NAME_DELIMITER_UNKNOWN);
}

bool SearchMatch::operator<(const SearchMatch& other) const {
  // score
  if(score > other.score) {
    return true;
  } else if(score < other.score) {
    return false;
  }

  const std::wstring* str = &text;
  const std::wstring* otherStr = &other.text;
  if(*str == *otherStr) {
    str = &name;
    otherStr = &other.name;
  }

  const size_t size = getTextSizeForSorting(str);
  const size_t otherSize = other.getTextSizeForSorting(otherStr);

  // text size
  if(size < otherSize) {
    return true;
  }
  if(size > otherSize) {
    return false;
  }
  if(str->size() < otherStr->size()) {
    return true;
  }
  if(str->size() > otherStr->size()) {
    return false;
  }

  // lower case
  for(size_t i = 0; i < str->size(); i++) {
    const auto value0 = towlower(static_cast<std::uint32_t>(str->at(i)));
    const auto value1 = towlower(static_cast<std::uint32_t>(otherStr->at(i)));
    if(value0 != value1) {
      return value0 < value1;
    } else {
      // alphabetical
      if(str->at(i) < otherStr->at(i)) {
        return true;
      } else if(str->at(i) > otherStr->at(i)) {
        return false;
      }
    }
  }

  return getSearchTypeName() < other.getSearchTypeName();
}

bool SearchMatch::operator==(const SearchMatch& other) const {
  return text == other.text && searchType == other.searchType;
}

size_t SearchMatch::getTextSizeForSorting(const std::wstring* str) {
  // check if templated symbol and only use size up to template stuff
  const size_t pos = str->find(L'<');
  if(pos != std::wstring::npos) {
    return pos;
  }

  return str->size();
}

bool SearchMatch::isValid() const {
  return searchType != SEARCH_NONE;
}

bool SearchMatch::isFilterCommand() const {
  return searchType == SEARCH_COMMAND && getCommandType() == COMMAND_NODE_FILTER;
}

void SearchMatch::print(std::wostream& ostream) const {
  ostream << name << '\n' << L'\t';
  size_t count = 0;
  for(const size_t index : indices) {
    while(count < index) {
      ++count;
      ostream << L' ';
    }
    ostream << L'^';
    ++count;
  }
  ostream << '\n';
}

std::wstring SearchMatch::getFullName() const {
  if(searchType == SEARCH_TOKEN && nodeType.isFile()) {
    return text;
  }

  return name;
}

std::wstring SearchMatch::getSearchTypeName() const {
  return getSearchTypeName(searchType);
}

SearchMatch::CommandType SearchMatch::getCommandType() const {
  if(name == L"overview") {
    return COMMAND_ALL;
  } else if(name == L"error") {
    return COMMAND_ERROR;
  } else if(name == L"legend") {
    return COMMAND_LEGEND;
  }

  return COMMAND_NODE_FILTER;
}
