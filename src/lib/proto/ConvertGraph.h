#pragma once
#include <memory>

#include "engine.pb.h"

class Graph;

namespace proto::convert {

/**
 * Graph <-> ProtoGraph.
 *
 * Both directions live here so client and server share one implementation; conversion written
 * inline at a call site drifts silently from its counterpart.
 *
 * Node/Edge carry TokenComponents that the GUI dereferences *without* null checks (see
 * DummyEdge::getBundledEdgesCount and GraphController's bundled-edge handling), so dropping a
 * component during conversion is a crash rather than a cosmetic loss. Every component produced by
 * PersistentStorage's graph builders is carried: Access, FilePath, IsAmbiguous, InheritanceChain
 * and BundledEdges.
 */
sourcetrail::ProtoGraph toProto(const Graph& graph);

/**
 * Rebuilds a Graph. Nodes are created before edges (an Edge needs its endpoint Node*), and
 * components are attached last. Returns an empty graph for an empty message; edges referencing
 * unknown nodes are skipped rather than dereferencing null.
 */
std::shared_ptr<Graph> fromProto(const sourcetrail::ProtoGraph& msg);

}    // namespace proto::convert
