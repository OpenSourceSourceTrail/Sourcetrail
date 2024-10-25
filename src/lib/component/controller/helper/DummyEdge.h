#pragma once

#include <QVector4D>

// internal
#include "Edge.h"
#include "TokenComponentBundledEdges.h"
#include "types.h"

class Edge;

// temporary data structure for (visual) graph creation process
struct DummyEdge {
  DummyEdge() = default;

  DummyEdge(const Id ownerId_, const Id targetId_, const Edge* data_) : ownerId(ownerId_), targetId(targetId_), data(data_) {}

  int getWeight() const {
    if(!data) {
      return weight;
    } else if(data->isType(Edge::EDGE_BUNDLED_EDGES)) {
      return data->getComponent<TokenComponentBundledEdges>()->getBundledEdgesCount();
    }

    return 1;
  }

  void updateDirection(TokenComponentBundledEdges::Direction dir, bool invert) {
    if(invert) {
      dir = TokenComponentBundledEdges::opposite(dir);
    }

    if(direction == TokenComponentBundledEdges::DIRECTION_INVALID) {
      direction = dir;
    } else if(direction != dir) {
      direction = TokenComponentBundledEdges::DIRECTION_NONE;
    }
  }

  TokenComponentBundledEdges::Direction getDirection() const {
    if(!data) {
      return direction;
    } else if(data->isType(Edge::EDGE_BUNDLED_EDGES)) {
      return data->getComponent<TokenComponentBundledEdges>()->getDirection();
    }

    return TokenComponentBundledEdges::DIRECTION_FORWARD;
  }

  Id ownerId = 0;
  Id targetId = 0;

  const Edge* data = nullptr;

  bool visible = false;
  bool hidden = false;
  bool active = false;

  std::vector<QVector4D> path;

  bool layoutHorizontal = true;

  // BundleEdge
  int weight = 0;
  TokenComponentBundledEdges::Direction direction = TokenComponentBundledEdges::DIRECTION_INVALID;
};
