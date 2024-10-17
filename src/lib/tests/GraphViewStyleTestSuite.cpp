#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "GraphViewStyle.h"
#include "GraphViewStyleImpl.h"
#include "NodeType.h"

using namespace testing;

struct MockedGraphViewStyleImpl : GraphViewStyleImpl {
  MOCK_METHOD(float, getCharWidth, (const std::string&, size_t), (override));
  MOCK_METHOD(float, getCharHeight, (const std::string&, size_t), (override));
  MOCK_METHOD(float, getGraphViewZoomDifferenceForPlatform, (), (override));
};

TEST(GraphViewStyle, Impl) {
  // No impls found
  ASSERT_EQ(nullptr, GraphViewStyle::getImpl());
  // Set impls
  auto graphViewStyle = std::make_shared<MockedGraphViewStyleImpl>();
  GraphViewStyle::setImpl(graphViewStyle);
  ASSERT_EQ(graphViewStyle, GraphViewStyle::getImpl());
  GraphViewStyle::setImpl(nullptr);
}

struct GraphViewStyleFix : Test {
  void SetUp() override {
    graphViewStyle = std::make_shared<MockedGraphViewStyleImpl>();
    GraphViewStyle::setImpl(graphViewStyle);
  }

  void TearDown() override {
    GraphViewStyle::setImpl(nullptr);
    graphViewStyle.reset();
  }

  std::shared_ptr<MockedGraphViewStyleImpl> graphViewStyle;
};

void PrintTo(const GraphViewStyle::NodeColor& node, std::ostream* os) {
  // clang-format off
  *os << "fill: "       << node.fill
      << ", text: "     << node.text
      << ", border: "   << node.border
      << ", hatching: " << node.hatching
      << ", icon: "     << node.icon;
  // clang-format on
}

TEST_F(GraphViewStyleFix, loadStyleSettings) {
  EXPECT_CALL(*graphViewStyle, getGraphViewZoomDifferenceForPlatform()).WillOnce(Return(0));
  EXPECT_CALL(*graphViewStyle, getCharHeight(_, _)).WillOnce(Return(0));

  GraphViewStyle::loadStyleSettings();
  EXPECT_EQ(11, GraphViewStyle::getFontSizeForStyleType(NodeType::StyleType::STYLE_PACKAGE));
  EXPECT_EQ(12, GraphViewStyle::getFontSizeOfAccessNode());
  EXPECT_EQ(11, GraphViewStyle::getFontSizeOfExpandToggleNode());
  EXPECT_EQ(11, GraphViewStyle::getFontSizeOfCountCircle());
  EXPECT_EQ(11, GraphViewStyle::getFontSizeOfQualifier());
  EXPECT_EQ(114, GraphViewStyle::getFontSizeOfTextNode(100));
  EXPECT_EQ(12, GraphViewStyle::getFontSizeOfGroupNode());

  EXPECT_EQ("Source Code Pro, consolas, monospace, sans-serif", GraphViewStyle::getFontNameForDataNode());
  EXPECT_EQ("Fira Sans, sans-serif", GraphViewStyle::getFontNameOfAccessNode());
  EXPECT_EQ("Fira Sans, sans-serif", GraphViewStyle::getFontNameOfExpandToggleNode());
  EXPECT_EQ("Fira Sans, sans-serif", GraphViewStyle::getFontNameOfTextNode());
  EXPECT_EQ("Source Code Pro, consolas, monospace, sans-serif", GraphViewStyle::getFontNameOfGroupNode());

  // EXPECT_EQ(GraphViewStyle::NodeMargins::bottom, GraphViewStyle::getMarginsForDataNode());
  /*
static NodeMargins getMarginsOfAccessNode(AccessKind access);
static NodeMargins getMarginsOfExpandToggleNode();
static NodeMargins getMarginsOfBundleNode();
static NodeMargins getMarginsOfTextNode(int fontSizeDiff);
static NodeMargins getMarginsOfGroupNode(GroupType type, bool hasName);

static NodeStyle getStyleForNodeType(NodeType type, bool defined, bool isActive, bool isFocused, bool isCoFocused, bool
hasChildren, bool hasQualifier); static NodeStyle getStyleOfAccessNode(); static NodeStyle getStyleOfExpandToggleNode(); static
NodeStyle getStyleOfCountCircle(); static NodeStyle getStyleOfBundleNode(bool isFocused); static NodeStyle getStyleOfQualifier();
static NodeStyle getStyleOfTextNode(int fontSizeDiff);
static NodeStyle getStyleOfGroupNode(GroupType type, bool isCoFocused);

static EdgeStyle getStyleForEdgeType(Edge::EdgeType type, bool isActive, bool isFocused, bool isTrailEdge, bool isAmbiguous);

static int toGridOffset(int x);
static int toGridSize(int x);
static int toGridGap(int x);
 */

  EXPECT_EQ(0, GraphViewStyle::getZoomFactor());
  EXPECT_EQ("#FF1493", GraphViewStyle::getFocusColor());

  // fill: #FF1493, text: #FF1493, border: #FF1493, hatching: #FF1493, icon: #FF1493
  EXPECT_EQ(GraphViewStyle::NodeColor{}, GraphViewStyle::getNodeColor("", true));

  /*
  static const NodeColor& getNodeColor(const std::string& typeStr, bool highlight);
  static const std::string& getEdgeColor(const std::string& type);
  static const NodeColor& getScreenMatchColor(bool focus);
     */
}

TEST(GraphViewStyle, loadStyleSettingsMissingImpl) {
  GraphViewStyle::loadStyleSettings();
}
