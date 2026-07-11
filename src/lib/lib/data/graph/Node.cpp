#include "Node.h"

#include <sstream>

#include "logging.h"
#include "TokenComponentAccess.h"
#include "TokenComponentConst.h"
#include "TokenComponentStatic.h"
#include "utilityString.h"

Node::Node(Id nodeId, NodeType type, NameHierarchy nameHierarchy, DefinitionKind definitionKind)
    : Token(nodeId), mType(type), mNameHierarchy(std::move(nameHierarchy)), mDefinitionKind(definitionKind) {}

Node::Node(const Node& other)
    : Token(other)
    , mType(other.mType)
    , mNameHierarchy(other.mNameHierarchy)
    , mDefinitionKind(other.mDefinitionKind)
    , mChildCount(other.mChildCount) {}

Node::~Node() = default;

void Node::setType(NodeType type) {
  if(!isType(type.getKind() | NODE_SYMBOL)) {    // NOLINT(hicpp-signed-bitwise)
    LOG_WARNING(L"Cannot change NodeType after it was already set from " + getReadableTypeString() + L" to " +
                type.getReadableTypeWString());
    return;
  }
  mType = type;
}

bool Node::isType(NodeKindMask mask) const {
  return (mType.getKind() & mask) > 0;    // NOLINT(hicpp-signed-bitwise)
}

std::wstring Node::getName() const {
  return mNameHierarchy.getRawName();
}

std::wstring Node::getFullName() const {
  return mNameHierarchy.getQualifiedName();
}

const NameHierarchy& Node::getNameHierarchy() const {
  return mNameHierarchy;
}

bool Node::isDefined() const {
  return mDefinitionKind != DEFINITION_NONE;
}

bool Node::isImplicit() const {
  return mDefinitionKind == DEFINITION_IMPLICIT;
}

bool Node::isExplicit() const {
  return mDefinitionKind == DEFINITION_EXPLICIT;
}

size_t Node::getChildCount() const {
  return mChildCount;
}

void Node::setChildCount(size_t childCount) {
  mChildCount = childCount;
}

size_t Node::getEdgeCount() const {
  return mEdges.size();
}

void Node::addEdge(Edge* edge) {
  mEdges.emplace(edge->getId(), edge);
}

void Node::removeEdge(Edge* edge) {
  auto iterator = mEdges.find(edge->getId());
  if(iterator != mEdges.end()) {
    mEdges.erase(iterator);
  }
}

Node* Node::getParentNode() const {
  if(Edge* edge = getMemberEdge(); nullptr != edge) {
    return edge->getFrom();
  }
  return nullptr;
}

Node* Node::getLastParentNode() {    // NOLINT(misc-no-recursion)
  if(Node* parent = getParentNode(); nullptr != parent) {
    return parent->getLastParentNode();
  }
  return this;
}

Edge* Node::getMemberEdge() const {
  return findEdgeOfType(Edge::EDGE_MEMBER, [this](Edge* edge) { return edge->getTo() == this; });
}

bool Node::isParentOf(const Node* node) const {
  while((node = node->getParentNode()) != nullptr) {
    if(node == this) {
      return true;
    }
  }
  return false;
}

Edge* Node::findEdge(const std::function<bool(Edge*)>& func) const {
  auto iterator = std::ranges::find_if(mEdges, [func](const std::pair<Id, Edge*>& pair) { return func(pair.second); });

  if(iterator != mEdges.end()) {
    return iterator->second;
  }

  return nullptr;
}

Edge* Node::findEdgeOfType(Edge::TypeMask mask) const {
  return findEdgeOfType(mask, [](Edge*) { return true; });
}

Edge* Node::findEdgeOfType(Edge::TypeMask mask, const std::function<bool(Edge*)>& func) const {
  auto iterator = std::ranges::find_if(mEdges, [mask, func](const std::pair<Id, Edge*>& pair) {
    if(pair.second->isType(mask)) {
      return func(pair.second);
    }
    return false;
  });

  if(iterator != mEdges.end()) {
    return iterator->second;
  }

  return nullptr;
}

Node* Node::findChildNode(const std::function<bool(Node*)>& func) const {
  auto iterator = std::ranges::find_if(mEdges, [&func](const std::pair<Id, Edge*>& item) {
    if(item.second->getType() == Edge::EDGE_MEMBER) {
      return func(item.second->getTo());
    }
    return false;
  });

  if(iterator != mEdges.end()) {
    return iterator->second->getTo();
  }

  return nullptr;
}

void Node::forEachEdge(const std::function<void(Edge*)>& func) const {
  std::ranges::for_each(mEdges, [func](const std::pair<Id, Edge*>& pair) { func(pair.second); });
}

void Node::forEachEdgeOfType(Edge::TypeMask mask, const std::function<void(Edge*)>& func) const {
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks): FIXME
  std::ranges::for_each(mEdges, [mask, func](const std::pair<Id, Edge*>& edgePair) {
    if(edgePair.second->isType(mask)) {
      func(edgePair.second);
    }
  });
}

void Node::forEachChildNode(const std::function<void(Node*)>& func) const {
  forEachEdgeOfType(Edge::EDGE_MEMBER, [func, this](Edge* edge) {
    if(this != edge->getTo()) {
      func(edge->getTo());
    }
  });
}

void Node::forEachNodeRecursive(const std::function<void(const Node*)>& func) const {
  func(this);

  forEachEdgeOfType(Edge::EDGE_MEMBER, [func, this](const Edge* edge) {
    if(this != edge->getTo()) {
      edge->getTo()->forEachNodeRecursive(func);
    }
  });
}

std::wstring Node::getReadableTypeString() const {
  return mType.getReadableTypeWString();
}

std::wstring Node::getAsString() const {
  std::wstringstream str;
  str << L"[" << getId() << L"] " << getReadableTypeString() << L": " << L"\"" << getName() << L"\"";

  auto* access = getComponent<TokenComponentAccess>();
  if(nullptr != access) {
    str << L" " << access->getAccessString();
  }

  if(nullptr != getComponent<TokenComponentStatic>()) {
    str << L" static";
  }

  if(nullptr != getComponent<TokenComponentConst>()) {
    str << L" const";
  }

  return str.str();
}

std::wostream& operator<<(std::wostream& ostream, const Node& node) {
  return ostream << node.getAsString();
}
