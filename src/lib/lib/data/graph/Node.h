#pragma once
#include <functional>
#include <map>
#include <string>

#include "DefinitionKind.h"
#include "Edge.h"
#include "NameHierarchy.h"
#include "NodeType.h"
#include "Token.h"

class Node : public Token {
public:
  Node(Id nodeId, NodeType type, NameHierarchy nameHierarchy, DefinitionKind definitionKind);
  Node(const Node& other);
  Node& operator=(const Node&) = delete;
  ~Node() override;

  NodeType getType() const {
    return mType;
  }
  void setType(NodeType type);
  bool isType(NodeKindMask mask) const;

  std::wstring getName() const;
  std::wstring getFullName() const;
  const NameHierarchy& getNameHierarchy() const;

  bool isDefined() const;
  bool isImplicit() const;
  bool isExplicit() const;

  size_t getChildCount() const;
  void setChildCount(size_t childCount);

  size_t getEdgeCount() const;

  void addEdge(Edge* edge);
  void removeEdge(Edge* edge);

  Node* getParentNode() const;
  Node* getLastParentNode();
  Edge* getMemberEdge() const;
  bool isParentOf(const Node* node) const;

  Edge* findEdge(const std::function<bool(Edge*)>& func) const;
  Edge* findEdgeOfType(Edge::TypeMask mask) const;
  Edge* findEdgeOfType(Edge::TypeMask mask, const std::function<bool(Edge*)>& func) const;
  Node* findChildNode(const std::function<bool(Node*)>& func) const;

  void forEachEdge(const std::function<void(Edge*)>& func) const;
  void forEachEdgeOfType(Edge::TypeMask mask, const std::function<void(Edge*)>& func) const;
  void forEachChildNode(const std::function<void(Node*)>& func) const;
  void forEachNodeRecursive(const std::function<void(const Node*)>& func) const;

  // Token implementation.
  [[nodiscard]] bool isNode() const override {
    return true;
  }
  [[nodiscard]] bool isEdge() const override {
    return false;
  }

  // Logging.
  virtual std::wstring getReadableTypeString() const override;
  std::wstring getAsString() const;

private:
  std::map<Id, Edge*> mEdges;

  NodeType mType;
  const NameHierarchy mNameHierarchy;
  DefinitionKind mDefinitionKind;

  size_t mChildCount = 0;
};

std::wostream& operator<<(std::wostream& ostream, const Node& node);
