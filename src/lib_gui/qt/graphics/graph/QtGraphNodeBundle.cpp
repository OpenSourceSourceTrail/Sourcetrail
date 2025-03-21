#include "QtGraphNodeBundle.h"

#include <QBrush>
#include <QPen>
#include <QVector2D>

#include "GraphViewStyle.h"
#include "QtCountCircleItem.h"
#include "type/graph/MessageGraphNodeBundleSplit.h"

QtGraphNodeBundle::QtGraphNodeBundle(
    GraphFocusHandler* focusHandler, Id tokenId, size_t nodeCount, NodeType type, const std::wstring& name, bool interactive)
    : QtGraphNode(focusHandler), m_circle(new QtCountCircleItem(this)), m_tokenId(tokenId), m_type(type) {
  m_isInteractive = interactive;

  this->setName(name);

  m_circle->setNumber(nodeCount);

  this->setAcceptHoverEvents(true);

  this->setToolTip(QStringLiteral("bundle"));
}

QtGraphNodeBundle::~QtGraphNodeBundle() = default;

bool QtGraphNodeBundle::isBundleNode() const {
  return true;
}

Id QtGraphNodeBundle::getTokenId() const {
  return m_tokenId;
}

void QtGraphNodeBundle::onClick() {
  MessageGraphNodeBundleSplit(m_tokenId,
                              (!m_type.isUnknownSymbol() || getName() == L"Symbols") &&
                                  getName() != L"Anonymous Namespaces",    // TODO: move to language package
                              !m_type.isUnknownSymbol() || getName() == L"Symbols")
      .dispatch();
}

void QtGraphNodeBundle::updateStyle() {
  GraphViewStyle::NodeStyle style;
  if(!m_type.isUnknownSymbol()) {
    style = GraphViewStyle::getStyleForNodeType(m_type, true, false, m_isFocused, m_isCoFocused, false, false);
  } else {
    style = GraphViewStyle::getStyleOfBundleNode(m_isFocused);
  }
  setStyle(style);

  QVector2D pos(static_cast<float>(m_rect->rect().right()), static_cast<float>(m_rect->rect().top() - 2));
  if(m_type.getNodeStyle() == NodeType::STYLE_BIG_NODE) {
    pos += {-2, 2};
  }
  m_circle->setPosition(pos);

  GraphViewStyle::NodeStyle accessStyle = GraphViewStyle::getStyleOfCountCircle();
  m_circle->setStyle(accessStyle.color.fill.c_str(),
                     accessStyle.color.text.c_str(),
                     accessStyle.color.border.c_str(),
                     static_cast<size_t>(style.borderWidth));
}


void QtGraphNodeBundle::hoverEnterEvent(QGraphicsSceneHoverEvent* /*event*/) {
  focusIn();
}

void QtGraphNodeBundle::hoverLeaveEvent(QGraphicsSceneHoverEvent* /*event*/) {
  focusOut();
}
