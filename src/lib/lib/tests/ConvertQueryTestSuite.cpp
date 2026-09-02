#include <gtest/gtest.h>

#include "bookmark/domain/BookmarkCategory.h"
#include "bookmark/domain/EdgeBookmark.h"
#include "bookmark/domain/NodeBookmark.h"
#include "ConvertQuery.h"
#include "data/ErrorFilter.h"
#include "data/ErrorInfo.h"
#include "data/location/SourceLocationFile.h"
#include "data/NodeType.h"
#include "data/NodeTypeSet.h"
#include "data/search/SearchMatch.h"
#include "data/storage/StorageStats.h"
#include "FileInfo.h"
#include "tooltip/domain/TooltipInfo.h"

using namespace proto::convert;

TEST(ConvertQuery, nodeKindMaskRoundTrips) {
  NodeTypeSet types = NodeTypeSet::none();
  types.add(NodeTypeSet(NodeType(NODE_CLASS)));
  types.add(NodeTypeSet(NodeType(NODE_METHOD)));
  types.add(NodeTypeSet(NodeType(NODE_UNION)));    // the highest bit, NODE_MAX_VALUE

  const NodeTypeSet restored = nodeTypeSetFromMask(toNodeKindMask(types));

  EXPECT_EQ(restored, types);
}

TEST(ConvertQuery, emptyNodeKindMaskRoundTrips) {
  EXPECT_TRUE(nodeTypeSetFromMask(toNodeKindMask(NodeTypeSet::none())).isEmpty());
}

TEST(ConvertQuery, searchMatchRoundTrips) {
  SearchMatch match;
  match.name = L"MyClass::method";
  match.text = L"method";
  match.subtext = L"MyClass::";
  match.tokenIds = {3, 5, 8};
  match.tokenNames = {NameHierarchy(L"MyClass", NAME_DELIMITER_CXX)};
  match.typeName = L"method";
  match.nodeType = NodeType(NODE_METHOD);
  match.searchType = SearchMatch::SEARCH_TOKEN;
  match.indices = {0, 2, 4};
  match.score = -17;
  match.hasChildren = true;

  const SearchMatch restored = fromProto(toProto(match));

  EXPECT_EQ(restored.name, match.name);
  EXPECT_EQ(restored.text, match.text);
  EXPECT_EQ(restored.subtext, match.subtext);
  EXPECT_EQ(restored.tokenIds, match.tokenIds);
  ASSERT_EQ(restored.tokenNames.size(), 1U);
  EXPECT_EQ(restored.tokenNames[0].getQualifiedName(), match.tokenNames[0].getQualifiedName());
  EXPECT_EQ(restored.typeName, match.typeName);
  EXPECT_EQ(restored.nodeType.getKind(), NODE_METHOD);
  EXPECT_EQ(restored.searchType, SearchMatch::SEARCH_TOKEN);
  EXPECT_EQ(restored.indices, match.indices);
  EXPECT_EQ(restored.score, -17);
  EXPECT_TRUE(restored.hasChildren);
  // Derived from `name`, so it survives without its own field.
  EXPECT_EQ(restored.getCommandType(), match.getCommandType());
}

TEST(ConvertQuery, errorInfoRoundTrips) {
  const ErrorInfo error(9, L"undefined reference", L"/src/a.cpp", 12, 34, L"/src/a.cpp", true, false);

  const ErrorInfo restored = fromProto(toProto(error));

  EXPECT_EQ(restored.id, 9U);
  EXPECT_EQ(restored.message, L"undefined reference");
  EXPECT_EQ(restored.filePath, L"/src/a.cpp");
  EXPECT_EQ(restored.lineNumber, 12U);
  EXPECT_EQ(restored.columnNumber, 34U);
  EXPECT_EQ(restored.translationUnit, L"/src/a.cpp");
  EXPECT_TRUE(restored.fatal);
  EXPECT_FALSE(restored.indexed);
}

TEST(ConvertQuery, errorFilterRoundTrips) {
  ErrorFilter filter;
  filter.error = false;
  filter.fatal = true;
  filter.unindexedError = false;
  filter.unindexedFatal = true;
  filter.limit = 7;

  EXPECT_EQ(fromProto(toProto(filter)), filter);
}

TEST(ConvertQuery, fileInfoRoundTrips) {
  const FileInfo info(FilePath(L"/src/main.cpp"), TimeStamp("2026-08-08 11:22:33"));

  const FileInfo restored = fromProto(toProto(info));

  EXPECT_EQ(restored.path.wstr(), L"/src/main.cpp");
  EXPECT_EQ(restored.lastWriteTime.toString(), info.lastWriteTime.toString());
}

TEST(ConvertQuery, storageStatsRoundTrips) {
  StorageStats stats;
  stats.nodeCount = 100;
  stats.edgeCount = 200;
  stats.fileCount = 5;
  stats.completedFileCount = 4;
  stats.fileLOCCount = 1234;
  stats.timestamp = TimeStamp("2026-08-08 11:22:33");

  const StorageStats restored = fromProto(toProto(stats));

  EXPECT_EQ(restored.nodeCount, 100U);
  EXPECT_EQ(restored.edgeCount, 200U);
  EXPECT_EQ(restored.fileCount, 5U);
  EXPECT_EQ(restored.completedFileCount, 4U);
  EXPECT_EQ(restored.fileLOCCount, 1234U);
  EXPECT_EQ(restored.timestamp.toString(), stats.timestamp.toString());
}

TEST(ConvertQuery, nodeBookmarkRoundTrips) {
  const BookmarkCategory category(3, L"favourites");
  NodeBookmark bookmark(11, L"entry point", L"start reading here", TimeStamp("2026-08-08 11:22:33"), category);
  bookmark.setNodeIds({4, 6});

  const NodeBookmark restored = fromProto(toProto(bookmark));

  EXPECT_EQ(restored.getId(), 11U);
  EXPECT_EQ(restored.getName(), L"entry point");
  EXPECT_EQ(restored.getComment(), L"start reading here");
  EXPECT_EQ(restored.getTimeStamp().toString(), bookmark.getTimeStamp().toString());
  EXPECT_EQ(restored.getCategory().getId(), 3U);
  EXPECT_EQ(restored.getCategory().getName(), L"favourites");
  EXPECT_EQ(restored.getNodeIds(), (std::vector<Id>{4, 6}));
}

TEST(ConvertQuery, edgeBookmarkRoundTrips) {
  const BookmarkCategory category(0, L"");
  EdgeBookmark bookmark(12, L"call", L"", TimeStamp("2026-08-08 11:22:33"), category);
  bookmark.setEdgeIds({7});
  bookmark.setActiveNodeId(21);

  const EdgeBookmark restored = fromProto(toProto(bookmark));

  EXPECT_EQ(restored.getId(), 12U);
  EXPECT_EQ(restored.getName(), L"call");
  EXPECT_EQ(restored.getEdgeIds(), (std::vector<Id>{7}));
  EXPECT_EQ(restored.getActiveNodeId(), 21U);
}

TEST(ConvertQuery, tooltipInfoRoundTrips) {
  TooltipInfo info;
  info.title = L"int foo(int)";
  info.count = 2;
  info.countText = "references";
  info.offset = Vec2f(3.5F, -4.25F);

  TooltipSnippet snippet;
  snippet.code = L"int foo(int a);";
  snippet.locationFile = std::make_shared<SourceLocationFile>(FilePath(L"/src/foo.h"), L"cpp", false, true, true);
  snippet.locationFile->addSourceLocation(LOCATION_TOKEN, 1, {2}, 1, 5, 1, 7);
  info.snippets.push_back(snippet);

  const TooltipInfo restored = fromProto(toProto(info));

  EXPECT_EQ(restored.title, info.title);
  EXPECT_EQ(restored.count, 2);
  EXPECT_EQ(restored.countText, "references");
  EXPECT_FLOAT_EQ(restored.offset.x, 3.5F);
  EXPECT_FLOAT_EQ(restored.offset.y, -4.25F);
  ASSERT_EQ(restored.snippets.size(), 1U);
  EXPECT_EQ(restored.snippets[0].code, L"int foo(int a);");
  ASSERT_NE(restored.snippets[0].locationFile, nullptr);
  EXPECT_EQ(restored.snippets[0].locationFile->getFilePath().wstr(), L"/src/foo.h");
  EXPECT_EQ(restored.snippets[0].locationFile->getSourceLocationCount(), 1U);
}

TEST(ConvertQuery, tooltipSnippetWithoutLocationFileStaysNull) {
  TooltipInfo info;
  info.snippets.push_back(TooltipSnippet{L"code", nullptr});

  const TooltipInfo restored = fromProto(toProto(info));

  ASSERT_EQ(restored.snippets.size(), 1U);
  EXPECT_EQ(restored.snippets[0].locationFile, nullptr);
}
