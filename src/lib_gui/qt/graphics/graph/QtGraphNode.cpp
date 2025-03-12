#include "QtGraphNode.h"

#include <cmath>
#include <limits>

#include <QBrush>
#include <QCursor>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsSceneEvent>
#include <QPen>

#include "GraphFocusHandler.h"
#include "QtDeviceScaledPixmap.h"
#include "QtGraphEdge.h"
#include "QtGraphNodeComponent.h"
#include "QtGraphNodeExpandToggle.h"
#include "QtRoundedRectItem.h"
#include "ResourcePaths.h"
#include "type/code/MessageCodeShowDefinition.h"
#include "type/graph/MessageGraphNodeHide.h"
#include "type/graph/MessageGraphNodeMove.h"
#include "utilityQt.h"
#include "utilityString.h"

void QtGraphNode::blendIn() {
  setOpacity(1.0);
}

void QtGraphNode::blendOut() {
  setOpacity(0.0);
}

void QtGraphNode::showNode() {
  this->show();
}

void QtGraphNode::hideNode() {
  this->hide();
}

QtGraphNode* QtGraphNode::findNodeRecursive(const std::list<QtGraphNode*>& nodes, Id tokenId) {
  for(QtGraphNode* node : nodes) {
    if(node->getTokenId() == tokenId) {
      return node;
    }

    QtGraphNode* result = findNodeRecursive(node->getSubNodes(), tokenId);
    if(result != nullptr) {
      return result;
    }
  }

  return nullptr;
}

QtGraphNode::QtGraphNode(GraphFocusHandler* focusHandler) : m_focusHandler(focusHandler) {
  this->setPen(QPen(Qt::transparent));
  this->setCursor(Qt::PointingHandCursor);

  m_text = new QGraphicsSimpleTextItem(this);
  m_rect = new QtRoundedRectItem(this);
  m_undefinedRect = new QtRoundedRectItem(this);
  m_undefinedRect->hide();
}

QtGraphNode::~QtGraphNode() {}

QtGraphNode* QtGraphNode::getParent() const {
  return m_parentNode;
}

QtGraphNode* QtGraphNode::getLastParent(bool noGroups) const {
  QtGraphNode* node = const_cast<QtGraphNode*>(this);
  while(true) {
    QtGraphNode* parent = dynamic_cast<QtGraphNode*>(node->parentItem());
    if(!parent || (noGroups && parent->isGroupNode())) {
      break;
    }
    node = parent;
  }
  return node;
}

QtGraphNode* QtGraphNode::getLastNonGroupParent() const {
  return getLastParent(true);
}

void QtGraphNode::setParent(QtGraphNode* parentNode) {
  m_parentNode = parentNode;

  if(m_parentNode != nullptr) {
    QGraphicsRectItem::setParentItem(m_parentNode);
  }
}

const std::list<QtGraphNode*>& QtGraphNode::getSubNodes() const {
  return m_subNodes;
}

QVector2D QtGraphNode::getPosition() const {
  return {static_cast<float>(this->scenePos().x()), static_cast<float>(this->scenePos().y())};
}

bool QtGraphNode::setPosition(const QVector2D& position) {
  QVector2D currentPosition = getPosition();
  QVector2D offset = position - currentPosition;

  if(std::fabs(offset.x()) >= std::numeric_limits<float>::epsilon() ||
     std::fabs(offset.y()) >= std::numeric_limits<float>::epsilon()) {
    this->moveBy(offset.x(), offset.y());
    setColumnSize({});
    notifyEdgesAfterMove();
    return true;
  }

  return false;
}

const QVector2D& QtGraphNode::getSize() const {
  return m_size;
}

void QtGraphNode::setSize(const QVector2D& size) {
  m_size = size;

  this->setRect(0, 0, size.x(), size.y());
  m_rect->setRect(0, 0, size.x(), size.y());
  m_undefinedRect->setRect(1, 1, size.x() - 2, size.y() - 2);
}

const QVector2D& QtGraphNode::getColumnSize() const {
  return m_columnSize;
}

void QtGraphNode::setColumnSize(const QVector2D& size) {
  m_columnSize = size;
}

QSize QtGraphNode::size() const {
  return QSize(static_cast<int>(m_size.x()), static_cast<int>(m_size.y()));
}

void QtGraphNode::setSize(QSize size) {
  setSize(QVector2D{static_cast<float>(size.width()), static_cast<float>(size.height())});
}

QVector4D QtGraphNode::getBoundingRect() const {
  QVector2D pos = getPosition();
  QVector2D size = getSize();
  return {pos.x(), pos.y(), pos.x() + size.x(), pos.y() + size.y()};
}

void QtGraphNode::addOutEdge(QtGraphEdge* edge) {
  m_outEdges.push_back(edge);
}

void QtGraphNode::addInEdge(QtGraphEdge* edge) {
  m_inEdges.push_back(edge);
}

size_t QtGraphNode::getOutEdgeCount() const {
  return m_outEdges.size();
}

size_t QtGraphNode::getInEdgeCount() const {
  return m_inEdges.size();
}

bool QtGraphNode::getIsActive() const {
  return m_isActive;
}

void QtGraphNode::setIsActive(bool isActive) {
  m_isActive = isActive;
}

void QtGraphNode::setMultipleActive(bool multipleActive) {
  m_multipleActive = multipleActive;
}

bool QtGraphNode::hasActiveChild() const {
  if(m_isActive) {
    return true;
  }

  for(auto subNode : m_subNodes) {
    if(subNode->hasActiveChild()) {
      return true;
    }
  }

  return false;
}

bool QtGraphNode::isFocusable() const {
  return m_isInteractive && (isDataNode() || isGroupNode() || isBundleNode());
}

std::wstring QtGraphNode::getName() const {
  return m_text->text().toStdWString();
}

void QtGraphNode::setName(const std::wstring& name) {
  m_text->setText(QString::fromStdWString(name));
}

void QtGraphNode::addComponent(const std::shared_ptr<QtGraphNodeComponent>& component) {
  m_components.push_back(component);
}

void QtGraphNode::hoverEnter() {
  hoverEnterEvent(nullptr);

  QtGraphNode* parent = getParent();
  if(parent) {
    parent->hoverEnter();
  }
}

bool QtGraphNode::getIsFocused() const {
  return m_isFocused;
}

void QtGraphNode::setIsFocused(bool focused) {
  if(m_isFocused != focused) {
    m_isFocused = focused;

    if(focused) {
      coFocusIn();
    } else {
      coFocusOut();
    }

    updateStyle();
  }
}

void QtGraphNode::focusIn() {
  if(m_isInteractive && m_focusHandler) {
    m_focusHandler->focusNode(this);
  } else {
    coFocusIn();
  }
}

void QtGraphNode::focusOut() {
  if(m_isInteractive && m_focusHandler) {
    m_focusHandler->defocusNode(this);
  } else {
    coFocusOut();
  }
}

void QtGraphNode::coFocusIn() {
  if(m_isCoFocused) {
    return;
  }

  m_isCoFocused = true;

  forEachEdge([](QtGraphEdge* edge) {
    if(edge->isTrailEdge()) {
      edge->coFocusIn();
    }
  });

  updateStyle();
}

void QtGraphNode::coFocusOut() {
  if(!m_isCoFocused) {
    return;
  }

  m_isCoFocused = false;

  forEachEdge([](QtGraphEdge* edge) {
    if(edge->isTrailEdge()) {
      edge->coFocusOut();
    }
  });

  updateStyle();
}

void QtGraphNode::showNodeRecursive() {
  blendIn();
  showNode();

  for(auto subNode : m_subNodes) {
    subNode->showNodeRecursive();
  }
}

void QtGraphNode::matchNameRecursive(const std::wstring& query, std::vector<QtGraphNode*>* matchedNodes) {
  matchName(query, matchedNodes);

  for(auto subNode : m_subNodes) {
    subNode->matchNameRecursive(query, matchedNodes);
  }
}

void QtGraphNode::removeNameMatch() {
  if(m_matchLength) {
    m_matchRect->hide();
    m_matchText->hide();

    m_matchPos = 0;
    m_matchLength = 0;

    updateStyle();
  }
}

void QtGraphNode::setActiveMatch(bool active) {
  m_isActiveMatch = active;
}

bool QtGraphNode::isDataNode() const {
  return false;
}

bool QtGraphNode::isAccessNode() const {
  return false;
}

bool QtGraphNode::isExpandToggleNode() const {
  return false;
}

bool QtGraphNode::isBundleNode() const {
  return false;
}

bool QtGraphNode::isQualifierNode() const {
  return false;
}

bool QtGraphNode::isTextNode() const {
  return false;
}

bool QtGraphNode::isGroupNode() const {
  return false;
}

Id QtGraphNode::getTokenId() const {
  return 0;
}

void QtGraphNode::addSubNode(QtGraphNode* node) {
  m_subNodes.push_back(node);

  // push parent nodes to back so all edges going to the active subnode are visible
  if(node->getIsActive()) {
    QtGraphNode* parent = this;
    while(parent && !parent->isGroupNode()) {
      parent->setZValue(-10.0);
      parent->m_text->setZValue(-9.0);
      parent->m_rect->setZValue(-10.0);
      parent->m_undefinedRect->setZValue(-10.0);

      parent = parent->getParent();
    }
  }
}

void QtGraphNode::moved(const QVector2D& oldPosition) {
  setPosition(GraphViewStyle::alignOnRaster(getPosition()));

  if(isDataNode() || isGroupNode() || isBundleNode()) {
    MessageGraphNodeMove(getTokenId(), getPosition() - oldPosition).dispatch();
  }
}

void QtGraphNode::onClick() {}

void QtGraphNode::onMiddleClick() {}

void QtGraphNode::onHide() {
  Id tokenId = getTokenId();

  if(tokenId) {
    MessageGraphNodeHide(tokenId).dispatch();
  } else if(getParent()) {
    getParent()->onHide();
  }
}

Id QtGraphNode::onCollapseExpand() {
  for(auto subNode : getSubNodes()) {
    if(subNode->isExpandToggleNode()) {
      subNode->onClick();
      return getTokenId();
    }
  }

  if(getParent()) {
    return getParent()->onCollapseExpand();
  }

  return 0;
}

void QtGraphNode::onShowDefinition(bool inIDE) {
  Id tokenId = getTokenId();

  if(tokenId) {
    MessageCodeShowDefinition(tokenId, inIDE).dispatch();
  } else if(getParent()) {
    getParent()->onShowDefinition(inIDE);
  }
}

void QtGraphNode::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  event->ignore();

  for(std::shared_ptr<QtGraphNodeComponent> component : m_components) {
    component->nodeMousePressEvent(event);
  }

  if(!event->isAccepted()) {
    QtGraphNode* parent = getParent();
    if(parent) {
      parent->mousePressEvent(event);
    }
  }
}

void QtGraphNode::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  event->ignore();

  for(std::shared_ptr<QtGraphNodeComponent> component : m_components) {
    component->nodeMouseMoveEvent(event);
  }

  if(!event->isAccepted()) {
    QtGraphNode* parent = getParent();
    if(parent) {
      parent->mouseMoveEvent(event);
    }
  }
}

void QtGraphNode::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  event->ignore();

  for(std::shared_ptr<QtGraphNodeComponent> component : m_components) {
    component->nodeMouseReleaseEvent(event);
  }

  if(!event->isAccepted()) {
    QtGraphNode* parent = getParent();
    if(parent) {
      parent->mouseReleaseEvent(event);
    }
  }
}

void QtGraphNode::forEachEdge(std::function<void(QtGraphEdge*)> func) {
  for(QtGraphEdge* edge : m_outEdges) {
    func(edge);
  }

  for(QtGraphEdge* edge : m_inEdges) {
    func(edge);
  }
}

void QtGraphNode::notifyEdgesAfterMove() {
  forEachEdge([](QtGraphEdge* edge) {
    edge->clearPath();
    edge->updateLine();
  });

  for(QtGraphNode* node : m_subNodes) {
    node->notifyEdgesAfterMove();
  }
}

void QtGraphNode::matchName(const std::wstring& query, std::vector<QtGraphNode*>* matchedNodes) {
  m_isActiveMatch = false;
  const std::wstring name = getName();
  size_t pos = utility::toLowerCase(name).find(query);

  if(pos != std::string::npos) {
    if(!m_matchText) {
      m_matchRect = new QtRoundedRectItem(this);
      m_matchText = new QGraphicsSimpleTextItem(this);
    }

    m_matchRect->show();
    m_matchText->show();

    std::wstring matchName(name.length(), L' ');
    matchName.replace(pos, query.length(), name.substr(pos, query.length()));
    m_matchText->setText(QString::fromStdWString(matchName));

    matchedNodes->push_back(this);

    m_matchPos = pos;
    m_matchLength = query.length();

    updateStyle();
  } else if(m_matchLength) {
    removeNameMatch();
  }
}

void QtGraphNode::setStyle(const GraphViewStyle::NodeStyle& style) {
  QPen pen(Qt::transparent);
  if(style.borderWidth > 0) {
    pen.setColor(style.color.border.c_str());
    pen.setWidthF(style.borderWidth);
    if(style.borderDashed) {
      pen.setStyle(Qt::DashLine);
    }
  }

  m_rect->setPen(pen);
  m_rect->setBrush(QBrush(style.color.fill.c_str()));

  qreal radius = style.cornerRadius;
  m_rect->setRadius(radius);

  if(style.hasHatching) {
    QtDeviceScaledPixmap pattern(
        QString::fromStdWString(ResourcePaths::getGuiDirectoryPath().concatenate(L"graph_view/images/pattern.png").wstr()));
    pattern.scaleToHeight(12);
    QPixmap pixmap = utility::colorizePixmap(pattern.pixmap(), style.color.hatching.c_str());

    pen.setWidth(0);
    pen.setColor(Qt::transparent);

    m_undefinedRect->setPen(pen);
    m_undefinedRect->setBrush(pixmap);
    m_undefinedRect->setRadius(radius);
    m_undefinedRect->show();
  }

  if(!m_icon && !style.iconPath.empty()) {
    QtDeviceScaledPixmap pixmap(QString::fromStdWString(style.iconPath.wstr()));
    pixmap.scaleToHeight(static_cast<int>(style.iconSize));

    m_icon = new QGraphicsPixmapItem(utility::colorizePixmap(pixmap.pixmap(), style.color.icon.c_str()), this);
    m_icon->setTransformationMode(Qt::SmoothTransformation);
    m_icon->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    m_icon->setPos(style.iconOffset.x(), style.iconOffset.y());
  }

  QFont font(style.fontName.c_str());
  font.setPixelSize(static_cast<int>(style.fontSize));
  if(style.fontBold) {
    font.setWeight(QFont::Bold);
  }

  m_text->setFont(font);
  m_text->setBrush(QBrush(style.color.text.c_str()));
  m_text->setPos(static_cast<qreal>(style.iconOffset.x() + static_cast<float>(style.iconSize) + style.textOffset.x()),
                 static_cast<qreal>(style.textOffset.y()));

  if(m_matchLength) {
    GraphViewStyle::NodeColor color = GraphViewStyle::getScreenMatchColor(m_isActiveMatch);

    m_matchText->setFont(font);
    m_matchText->setBrush(QBrush(color.text.c_str()));
    m_matchText->setPos(static_cast<qreal>(style.iconOffset.x() + static_cast<float>(style.iconSize) + style.textOffset.x()),
                        static_cast<qreal>(style.textOffset.y()));

    const float charWidth = static_cast<float>(
                                QFontMetrics(font).boundingRect(QStringLiteral("QtGraphNode::QtGraphNode::QtGraphNode")).width()) /
        37.0f;
    const float charHeight = static_cast<float>(QFontMetrics(font).height());
    m_matchRect->setRect(static_cast<qreal>(style.iconOffset.x() + static_cast<float>(style.iconSize) + style.textOffset.x() +
                                            static_cast<float>(m_matchPos) * charWidth),
                         static_cast<qreal>(style.textOffset.y()),
                         static_cast<qreal>(static_cast<float>(m_matchLength) * charWidth),
                         static_cast<qreal>(charHeight));
    m_matchRect->setPen(QPen(color.border.c_str()));
    m_matchRect->setBrush(QBrush(color.fill.c_str()));
    m_matchRect->setRadius(3);
  }
}
