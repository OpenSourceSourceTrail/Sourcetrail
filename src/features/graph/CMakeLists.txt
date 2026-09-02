# ${PROJECT_SOURCE_DIR}/src/features/graph/CMakeLists.txt
#
# The graph feature: the token graph itself, its layout, and the scene that draws it. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning. Migrated last, because it
# is the largest and the one the headless engine reaches into.
#
# GraphViewStyle, the three layouters and Dummy{Node,Edge} are in logic/, NOT ui/, and that is not
# an oversight. src/app/engine/GraphLayoutService.cpp runs GraphController headlessly against a
# null-object view and calls GraphViewStyle::loadStyleSettings(); these types appear in
# src/lib/proto/engine.proto as the client/engine wire contract, and the client ships its own font
# metrics (engine.proto:276-285) precisely so layout is computed engine-side. Moving them to ui/
# would break the headless engine and the HTTP API. The toolkit split is already handled by
# GraphViewStyleImpl -- MetricGraphViewStyleImpl here, QtGraphViewStyleImpl in ui/.
#
# domain/ takes the graph vocabulary that had been spread across data/: not just Graph, Node, Edge
# and the token components, but NodeKind, NodeType, NodeTypeSet, GroupType, DefinitionKind and
# HierarchyCache. All of them are named by the storage layer and proto/Convert; that breadth is what
# the domain tier is for, and leaving them in data/ was what made the old data/ directory look like
# a tier when it was really a grab bag.

target_sources(
  Sourcetrail_lib
  PRIVATE domain/DefinitionKind.cpp
          domain/DefinitionKind.h
          domain/Edge.cpp
          domain/Edge.h
          domain/ElementComponentKind.cpp
          domain/ElementComponentKind.h
          domain/Graph.cpp
          domain/Graph.h
          domain/GroupType.cpp
          domain/GroupType.h
          domain/HierarchyCache.cpp
          domain/HierarchyCache.h
          domain/Node.cpp
          domain/Node.h
          domain/NodeKind.cpp
          domain/NodeKind.h
          domain/NodeType.cpp
          domain/NodeType.h
          domain/NodeTypeSet.cpp
          domain/NodeTypeSet.h
          domain/Token.cpp
          domain/Token.h
          domain/token_component/TokenComponent.cpp
          domain/token_component/TokenComponent.h
          domain/token_component/TokenComponentAbstraction.cpp
          domain/token_component/TokenComponentAbstraction.h
          domain/token_component/TokenComponentAccess.cpp
          domain/token_component/TokenComponentAccess.h
          domain/token_component/TokenComponentBundledEdges.cpp
          domain/token_component/TokenComponentBundledEdges.h
          domain/token_component/TokenComponentConst.cpp
          domain/token_component/TokenComponentConst.h
          domain/token_component/TokenComponentFilePath.cpp
          domain/token_component/TokenComponentFilePath.h
          domain/token_component/TokenComponentInheritanceChain.h
          domain/token_component/TokenComponentIsAmbiguous.h
          domain/token_component/TokenComponentStatic.cpp
          domain/token_component/TokenComponentStatic.h
          logic/BucketLayouter.cpp
          logic/BucketLayouter.h
          logic/DummyEdge.h
          logic/DummyNode.h
          logic/GraphController.cpp
          logic/GraphController.h
          logic/GraphView.cpp
          logic/GraphView.h
          logic/GraphViewStyle.cpp
          logic/GraphViewStyle.h
          logic/GraphViewStyleImpl.h
          logic/ListLayouter.cpp
          logic/ListLayouter.h
          logic/MetricGraphViewStyleImpl.h
          logic/TrailLayouter.cpp
          logic/TrailLayouter.h
          messages/MessageActivateEdge.h
          messages/MessageActivateNodes.h
          messages/MessageActivateTrailEdge.h
          messages/MessageDeactivateEdge.h
          messages/MessageGraphNodeBundleSplit.h
          messages/MessageGraphNodeExpand.h
          messages/MessageGraphNodeHide.h
          messages/MessageGraphNodeMove.h
          messages/MessageSaveAsImage.h
          messages/MessageScrollGraph.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/GraphFocusHandler.cpp
          ui/GraphFocusHandler.h
          ui/QtGraphView.cpp
          ui/QtGraphView.h
          ui/QtGraphViewStyleImpl.cpp
          ui/QtGraphViewStyleImpl.h
          ui/QtGraphicsView.cpp
          ui/QtGraphicsView.h
          ui/base/QtCountCircleItem.cpp
          ui/base/QtCountCircleItem.h
          ui/base/QtLineItemAngled.cpp
          ui/base/QtLineItemAngled.h
          ui/base/QtLineItemBase.cpp
          ui/base/QtLineItemBase.h
          ui/base/QtLineItemBezier.cpp
          ui/base/QtLineItemBezier.h
          ui/base/QtLineItemStraight.cpp
          ui/base/QtLineItemStraight.h
          ui/base/QtRoundedRectItem.cpp
          ui/base/QtRoundedRectItem.h
          ui/component/QtGraphNodeComponent.cpp
          ui/component/QtGraphNodeComponent.h
          ui/component/QtGraphNodeComponentClickable.cpp
          ui/component/QtGraphNodeComponentClickable.h
          ui/component/QtGraphNodeComponentMoveable.cpp
          ui/component/QtGraphNodeComponentMoveable.h
          ui/graph/QtGraphEdge.cpp
          ui/graph/QtGraphEdge.h
          ui/graph/QtGraphNode.cpp
          ui/graph/QtGraphNode.h
          ui/graph/QtGraphNodeAccess.cpp
          ui/graph/QtGraphNodeAccess.h
          ui/graph/QtGraphNodeBundle.cpp
          ui/graph/QtGraphNodeBundle.h
          ui/graph/QtGraphNodeData.cpp
          ui/graph/QtGraphNodeData.h
          ui/graph/QtGraphNodeExpandToggle.cpp
          ui/graph/QtGraphNodeExpandToggle.h
          ui/graph/QtGraphNodeGroup.cpp
          ui/graph/QtGraphNodeGroup.h
          ui/graph/QtGraphNodeQualifier.cpp
          ui/graph/QtGraphNodeQualifier.h
          ui/graph/QtGraphNodeText.cpp
          ui/graph/QtGraphNodeText.h)

if(ENABLE_UNIT_TEST)
  # GraphViewStyleTestSuite carries a TODO(SOUR-97) disabled case; it moved as-is.
  foreach(test_name IN ITEMS GraphControllerTestSuite GraphTestSuite GraphViewStyleTestSuite HierarchyCacheTestSuite)
    add_sourcetrail_test(
      NAME ${test_name}
      SOURCES tests/${test_name}.cpp
      DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
           Sourcetrail::lib::proto::convert
      TEST_PREFIX "unittests.lib.")

    target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)
  endforeach()
endif()
