#include "ReadSqliteBookmarkStorage.hpp"

#include <memory>

#include <CppSQLite3.h>

#include "logging.h"
#include "StorageBookmark.h"
#include "StorageBookmarkCategory.h"
#include "StorageBookmarkedEdge.h"
#include "StorageBookmarkedNode.h"

namespace {
constexpr auto GetAllBookmarksQuery{"SELECT id, name, comment, timestamp, category_id FROM bookmark;"};
constexpr auto GetAllBookmarkCategoriesQuery{"SELECT id, name FROM bookmark_category;"};
constexpr auto GetAllBookmarkCategoryWithNameQuery{L"SELECT id, name FROM bookmark_category WHERE name == '{}';"};
constexpr auto GetAllBookmarkedEdgesQuery =
    "SELECT "
    "bookmarked_node.id, bookmarked_element.bookmark_id, bookmarked_node.serialized_node_name "
    "FROM bookmarked_node "
    "INNER JOIN "
    "bookmarked_element ON bookmarked_node.id = bookmarked_element.id;";
constexpr auto GetAllBookmarkNodesQuery =
    "SELECT "
    "bookmarked_edge.id, bookmarked_element.bookmark_id, "
    "bookmarked_edge.serialized_source_node_name, bookmarked_edge.serialized_target_node_name, "
    "bookmarked_edge.edge_type, bookmarked_edge.source_node_active "
    "FROM bookmarked_edge "
    "INNER JOIN "
    "bookmarked_element ON bookmarked_edge.id = bookmarked_element.id;";
}    // namespace

namespace sqlite {
ReadSqliteBookmarkStorage::ReadSqliteBookmarkStorage() noexcept = default;

ReadSqliteBookmarkStorage::~ReadSqliteBookmarkStorage() noexcept {
  if(mDatabase) {
    try {
      mDatabase->close();
      mDatabase.reset();
    } catch(CppSQLite3Exception& exp) {
      LOG_WARNING(fmt::format("Failed to close database: {}", exp.what()));
    } catch(...) {
      LOG_WARNING(fmt::format("Failed to close database unknown error"));
    }
  }
}

expected<std::vector<StorageBookmark>> ReadSqliteBookmarkStorage::getAllBookmarks() const noexcept {
  CppSQLite3Query result;
  try {
    result = mDatabase->execQuery(GetAllBookmarksQuery);
  } catch(CppSQLite3Exception& exp) {
    return nonstd::unexpected<std::string>(exp.what());
  }

  std::vector<StorageBookmark> bookmarks;
  while(!result.eof()) {
    const Id id = static_cast<Id>(result.getIntField(0, 0));
    const std::string name = result.getStringField(1, "");
    const std::string comment = result.getStringField(2, "");
    const std::string timestamp = result.getStringField(3, "");
    const Id categoryId = result.getIntField(4, 0);

    if(id != 0 && !name.empty() && !timestamp.empty()) {
      bookmarks.emplace_back(id, utility::decodeFromUtf8(name), utility::decodeFromUtf8(comment), timestamp, categoryId);
    }

    result.nextRow();
  }
  return bookmarks;
}

expected<std::vector<StorageBookmarkedNode>> ReadSqliteBookmarkStorage::getAllBookmarkedNodes() const noexcept {
  CppSQLite3Query result;
  try {
    result = mDatabase->execQuery(GetAllBookmarkedEdgesQuery);
  } catch(CppSQLite3Exception& exp) {
    return nonstd::unexpected<std::string>(exp.what());
  }

  std::vector<StorageBookmarkedNode> bookmarkedNodes;
  while(!result.eof()) {
    const Id id = static_cast<Id>(result.getIntField(0, 0));
    const Id bookmarkId = static_cast<Id>(result.getIntField(1, 0));
    const std::string serializedNodeName = result.getStringField(2, "");

    if(id != 0 && bookmarkId != 0 && !serializedNodeName.empty()) {
      bookmarkedNodes.emplace_back(id, bookmarkId, utility::decodeFromUtf8(serializedNodeName));
    }

    result.nextRow();
  }
  return bookmarkedNodes;
}

expected<std::vector<StorageBookmarkedEdge>> ReadSqliteBookmarkStorage::getAllBookmarkedEdges() const noexcept {
  CppSQLite3Query result;
  try {
    result = mDatabase->execQuery(GetAllBookmarkNodesQuery);
  } catch(CppSQLite3Exception& exp) {
    return nonstd::unexpected<std::string>(exp.what());
  }

  std::vector<StorageBookmarkedEdge> bookmarkedEdge;
  while(!result.eof()) {
    const Id id = static_cast<Id>(result.getIntField(0, 0));
    const Id bookmarkId = static_cast<Id>(result.getIntField(1, 0));
    const std::string serializedSourceNodeName = result.getStringField(2, "");
    const std::string serializedTargetNodeName = result.getStringField(3, "");
    const int edgeType = result.getIntField(4, -1);
    const int sourceNodeActive = result.getIntField(5, -1);

    if(id != 0 && bookmarkId != 0 && !serializedSourceNodeName.empty() && !serializedTargetNodeName.empty() && edgeType != -1 &&
       sourceNodeActive != -1) {
      bookmarkedEdge.emplace_back(id,
                                  bookmarkId,
                                  utility::decodeFromUtf8(serializedSourceNodeName),
                                  utility::decodeFromUtf8(serializedTargetNodeName),
                                  edgeType,
                                  sourceNodeActive);
    }

    result.nextRow();
  }
  return bookmarkedEdge;
}

expected<std::vector<StorageBookmarkCategory>> ReadSqliteBookmarkStorage::getAllBookmarkCategories() const noexcept {
  CppSQLite3Query result;
  try {
    result = mDatabase->execQuery(GetAllBookmarkCategoriesQuery);
  } catch(CppSQLite3Exception& exp) {
    return nonstd::unexpected<std::string>(exp.what());
  }

  std::vector<StorageBookmarkCategory> categories;
  while(!result.eof()) {
    const Id id = static_cast<Id>(result.getIntField(0, 0));
    const std::string name = result.getStringField(1, "");

    if(id != 0 && !name.empty()) {
      categories.emplace_back(id, utility::decodeFromUtf8(name));
    }

    result.nextRow();
  }
  return categories;
}

expected<StorageBookmarkCategory> ReadSqliteBookmarkStorage::getBookmarkCategoryByName(const std::wstring& name) const noexcept {
  CppSQLite3Query result;
  try {
    const auto query = utility::encodeToUtf8(fmt::format(GetAllBookmarkCategoryWithNameQuery, name));
    result = mDatabase->execQuery(query.c_str());
  } catch(CppSQLite3Exception& exp) {
    return nonstd::unexpected<std::string>(exp.what());
  }

  while(!result.eof()) {
    const Id id = static_cast<Id>(result.getIntField(0, 0));
    const std::string categoryName = result.getStringField(1, "");

    if(id != 0 && !name.empty()) {
      return StorageBookmarkCategory{id, utility::decodeFromUtf8(categoryName)};
    }
  }
  return nonstd::unexpected<std::string>(
      fmt::format("No BookmarkCategory found by the following name: {}", utility::encodeToUtf8(name)));
}
}    // namespace sqlite
