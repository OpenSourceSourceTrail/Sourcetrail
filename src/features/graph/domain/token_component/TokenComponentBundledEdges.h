#ifndef TOKEN_COMPONENT_BUNDLED_EDGES_H
#define TOKEN_COMPONENT_BUNDLED_EDGES_H

#include <map>
#include <set>

#include "GlobalId.hpp"
#include "graph/domain/token_component/TokenComponent.h"

class TokenComponentBundledEdges : public TokenComponent {
public:
  enum Direction { DIRECTION_NONE, DIRECTION_FORWARD, DIRECTION_BACKWARD, DIRECTION_INVALID };

  static Direction opposite(Direction direction);

  TokenComponentBundledEdges();
  virtual ~TokenComponentBundledEdges();

  virtual std::shared_ptr<TokenComponent> copy() const;

  int getBundledEdgesCount() const;
  std::set<Id> getBundledEdgesIds() const;

  // Per-id directions. getBundledEdgesIds() collapses these away and getDirection() derives a
  // single value from them, so serialization needs this to reproduce the component faithfully.
  const std::map<Id, Direction>& getBundledEdgesWithDirection() const;

  void addBundledEdgesId(Id id, bool forward);
  void removeBundledEdgesId(Id id);

  Direction getDirection();

private:
  std::map<Id, Direction> m_ids;
  Direction m_direction;
};

#endif    // TOKEN_COMPONENT_BUNDLED_EDGES_H
