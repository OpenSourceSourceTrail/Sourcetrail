#include <gtest/gtest.h>

#include "Graph.h"

namespace {
class TestToken : public Token {
public:
  TestToken() : Token(0) {}

  TestToken(const TestToken& other) : Token(other) {}

  virtual bool isNode() const {
    return false;
  }

  virtual bool isEdge() const {
    return false;
  }

  void addComponent(std::shared_ptr<TokenComponent> component) {
    Token::addComponent(component);
  }

  template <typename ComponentType>
  std::shared_ptr<ComponentType> removeComponent() {
    return Token::removeComponent<ComponentType>();
  }

  virtual std::wstring getReadableTypeString() const {
    return L"";
  }
};

class TestComponent : public TokenComponent {
public:
  virtual std::shared_ptr<TokenComponent> copy() const {
    return std::make_shared<TestComponent>(*this);
  }
};

class Test2Component : public TokenComponent {
public:
  virtual std::shared_ptr<TokenComponent> copy() const {
    return std::make_shared<Test2Component>(*this);
  }
};
}    // namespace

TEST(Graph, tokensSaveLocationIds) {
  TestToken a;
  a.addLocationId(23);
  a.addLocationId(5);

  EXPECT_TRUE(a.getLocationIds().size() == 2);
  EXPECT_TRUE(a.getLocationIds()[0] == 23);
  EXPECT_TRUE(a.getLocationIds()[1] == 5);
}

TEST(Graph, tokensRemoveLocationIds) {
  TestToken a;
  a.addLocationId(23);
  a.addLocationId(5);
  a.removeLocationId(42);
  a.removeLocationId(5);

  EXPECT_TRUE(a.getLocationIds().size() == 1);
  EXPECT_TRUE(a.getLocationIds()[0] == 23);
}

TEST(Graph, tokenSavesComponent) {
  TestToken a;
  std::shared_ptr<TestComponent> component = std::make_shared<TestComponent>();
  a.addComponent(component);

  EXPECT_TRUE(a.getComponent<TestComponent>());
  EXPECT_TRUE(!a.getComponent<Test2Component>());

  EXPECT_TRUE(a.getComponent<TestComponent>() == component.get());
}

TEST(Graph, tokenSavesMultipleComponents) {
  TestToken a;
  std::shared_ptr<TestComponent> component = std::make_shared<TestComponent>();
  std::shared_ptr<Test2Component> component2 = std::make_shared<Test2Component>();
  a.addComponent(component2);
  a.addComponent(component);

  EXPECT_TRUE(a.getComponent<TestComponent>());
  EXPECT_TRUE(a.getComponent<Test2Component>());

  EXPECT_TRUE(a.getComponent<TestComponent>() == component.get());
  EXPECT_TRUE(a.getComponent<Test2Component>() == component2.get());
}

TEST(Graph, tokenRemovesComponent) {
  TestToken a;
  std::shared_ptr<TestComponent> component = std::make_shared<TestComponent>();
  std::shared_ptr<Test2Component> component2 = std::make_shared<Test2Component>();
  a.addComponent(component2);
  a.addComponent(component);
  std::shared_ptr<TestComponent> component3 = a.removeComponent<TestComponent>();

  EXPECT_TRUE(!a.getComponent<TestComponent>());
  EXPECT_TRUE(a.getComponent<Test2Component>());

  EXPECT_TRUE(component3.get() == component.get());
  EXPECT_TRUE(a.getComponent<Test2Component>() == component2.get());
}

TEST(Graph, tokenCopiesComponentsWhenTokenIsCopied) {
  TestToken a;
  std::shared_ptr<TestComponent> component = std::make_shared<TestComponent>();
  std::shared_ptr<Test2Component> component2 = std::make_shared<Test2Component>();
  a.addComponent(component2);
  a.addComponent(component);

  TestToken b(a);

  EXPECT_TRUE(b.getComponent<TestComponent>());
  EXPECT_TRUE(b.getComponent<Test2Component>());

  EXPECT_TRUE(b.getComponent<TestComponent>() != component.get());
  EXPECT_TRUE(b.getComponent<Test2Component>() != component2.get());
}

TEST(Graph, nodesAreNodes) {
  Node a(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);

  EXPECT_TRUE(a.isNode());
  EXPECT_TRUE(!a.isEdge());
}

TEST(Graph, edgesAreEdges) {
  Node a(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node b(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Edge e(3, Edge::EDGE_USAGE, &a, &b);

  EXPECT_TRUE(!e.isNode());
  EXPECT_TRUE(e.isEdge());
}

TEST(Graph, setTypeOfNodeFromConstructor) {
  Node n(1, NodeType(NODE_FUNCTION), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  EXPECT_TRUE(NodeType(NODE_FUNCTION) == n.getType());
}

TEST(Graph, setTypeOfNodeFromNonIndexed) {
  Node n(2, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  n.setType(NodeType(NODE_CLASS));
  EXPECT_TRUE(NodeType(NODE_CLASS) == n.getType());
}

TEST(Graph, canNotChangeTypeOfNodeAfterItWasSet) {
  Node n(3, NodeType(NODE_NAMESPACE), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  n.setType(NodeType(NODE_CLASS));
  EXPECT_TRUE(NodeType(NODE_CLASS) != n.getType());
}

TEST(Graph, nodeCanBeCopiedAndKeepsSameId) {
  Node n(4, NodeType(NODE_NAMESPACE), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node n2(n);

  EXPECT_TRUE(&n != &n2);
  EXPECT_TRUE(n.getId() == n2.getId());
  EXPECT_TRUE(n.getName() == n2.getName());
  EXPECT_TRUE(n.getType() == n2.getType());
}

TEST(Graph, nodeTypeBitMasking) {
  Node n(1, NodeType(NODE_NAMESPACE), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  EXPECT_TRUE(n.isType(NODE_FUNCTION | NODE_NAMESPACE | NODE_CLASS));
  EXPECT_TRUE(!n.isType(NODE_FUNCTION | NODE_METHOD | NODE_CLASS));
}

TEST(Graph, getTypeOfEdges) {
  Node a(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node b(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Edge e(3, Edge::EDGE_USAGE, &a, &b);

  EXPECT_TRUE(Edge::EDGE_USAGE == e.getType());
}

TEST(Graph, edgeCanBeCopiedAndKeepsSameId) {
  Node a(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node b(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Edge e(3, Edge::EDGE_USAGE, &a, &b);
  Edge e2(e, &a, &b);

  EXPECT_TRUE(&e != &e2);
  EXPECT_TRUE(e.getId() == e2.getId());
  EXPECT_TRUE(e.getType() == e2.getType());
}

TEST(Graph, edgeTypeBitMasking) {
  Node a(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node b(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Edge e(3, Edge::EDGE_USAGE, &a, &b);

  EXPECT_TRUE(e.isType(Edge::EDGE_MEMBER | Edge::EDGE_CALL | Edge::EDGE_USAGE));
  EXPECT_TRUE(!e.isType(Edge::EDGE_MEMBER | Edge::EDGE_CALL));
}

TEST(Graph, nodeFindsChildNode) {
  Node a(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node b(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node c(3, NodeType(NODE_SYMBOL), NameHierarchy(L"C", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Edge e(4, Edge::EDGE_MEMBER, &a, &b);
  Edge e2(5, Edge::EDGE_MEMBER, &a, &c);

  Node* x = a.findChildNode([](Node* n) { return n->getName() == L"C"; });

  EXPECT_TRUE(x == &c);
  EXPECT_TRUE(x != &b);
}

TEST(Graph, nodeCanNotFindChildNode) {
  Node a(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node b(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node c(3, NodeType(NODE_SYMBOL), NameHierarchy(L"C", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Edge e(4, Edge::EDGE_MEMBER, &a, &b);
  Edge e2(5, Edge::EDGE_MEMBER, &a, &c);

  Node* x = a.findChildNode([](Node* n) { return n->getName() == L"D"; });

  EXPECT_TRUE(!x);
}

TEST(Graph, nodeVisitsChildNodes) {
  Node a(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node b(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node c(3, NodeType(NODE_SYMBOL), NameHierarchy(L"C", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Edge e(4, Edge::EDGE_MEMBER, &a, &b);
  Edge e2(5, Edge::EDGE_MEMBER, &a, &c);

  std::vector<Node*> children;
  a.forEachChildNode([&children](Node* n) { return children.push_back(n); });

  EXPECT_TRUE(children.size() == 2);
  EXPECT_TRUE(children[0] == &b);
  EXPECT_TRUE(children[1] == &c);
}

TEST(Graph, graphSavesNodes) {
  Graph graph;
  Node* a = graph.createNode(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node* b = graph.createNode(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);

  EXPECT_TRUE(2 == graph.getNodeCount());
  EXPECT_TRUE(0 == graph.getEdgeCount());

  EXPECT_TRUE(graph.getNodeById(a->getId()));
  EXPECT_TRUE(L"A" == graph.getNodeById(a->getId())->getName());

  EXPECT_TRUE(graph.getNodeById(b->getId()));
  EXPECT_TRUE(L"B" == graph.getNodeById(b->getId())->getName());

  EXPECT_TRUE(!graph.getNodeById(0));
}

TEST(Graph, graphSavesEdges) {
  Graph graph;

  Node* a = graph.createNode(1, NodeType(NODE_FUNCTION), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  Node* b = graph.createNode(2, NodeType(NODE_FUNCTION), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);

  Edge* e = graph.createEdge(3, Edge::EDGE_CALL, a, b);

  EXPECT_TRUE(2 == graph.getNodeCount());
  EXPECT_TRUE(1 == graph.getEdgeCount());

  EXPECT_TRUE(graph.getEdgeById(e->getId()));
  EXPECT_TRUE(Edge::EDGE_CALL == graph.getEdgeById(e->getId())->getType());
}

TEST(Graph, graphRemovesNodes) {
  Graph graph;

  Node* a = graph.createNode(1, NodeType(NODE_SYMBOL), NameHierarchy(L"A", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);
  graph.createNode(2, NodeType(NODE_SYMBOL), NameHierarchy(L"B", NAME_DELIMITER_CXX), DEFINITION_EXPLICIT);

  EXPECT_TRUE(2 == graph.getNodeCount());
  EXPECT_TRUE(0 == graph.getEdgeCount());

  graph.removeNode(graph.getNodeById(a->getId()));

  EXPECT_TRUE(1 == graph.getNodeCount());
}
