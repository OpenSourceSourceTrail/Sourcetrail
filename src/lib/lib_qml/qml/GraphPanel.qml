import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import Sourcetrail

/**
 * The graph canvas.
 *
 * Everything here is painting. Positions, sizes, colours, fonts and edge paths all arrive resolved
 * from C++, because they come out of the same GraphController and GraphViewStyle the engine daemon
 * serves over /api/v1/graph/layout -- a second layout or a second palette in QML would be a second
 * answer to the same question.
 */
Item {
    id: root

    // DummyNode::Type, mirrored so the delegates can branch on what they are painting.
    readonly property int typeData: 0
    readonly property int typeAccess: 1
    readonly property int typeExpandToggle: 2
    readonly property int typeBundle: 3
    readonly property int typeQualifier: 4
    readonly property int typeText: 5
    readonly property int typeGroup: 6

    readonly property real minZoom: 0.15
    readonly property real maxZoom: 4.0

    // BucketLayouter divides the viewport into columns, so the controller has to know how big the
    // canvas is before it lays anything out.
    onWidthChanged: GraphViewModel.setViewportSize(width, height)
    onHeightChanged: GraphViewModel.setViewportSize(width, height)
    Component.onCompleted: GraphViewModel.setViewportSize(width, height)

    function centerOn(x, y, w, h) {
        flick.contentX = Math.max(0, (x - GraphViewModel.boundsX + w / 2) * canvas.scale - flick.width / 2)
        flick.contentY = Math.max(0, (y - GraphViewModel.boundsY + h / 2) * canvas.scale - flick.height / 2)
    }

    Connections {
        target: GraphViewModel
        function onCenterRequested(x, y, w, h) { root.centerOn(x, y, w, h) }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentWidth: canvas.width * canvas.scale
        contentHeight: canvas.height * canvas.scale
        boundsBehavior: Flickable.StopAtBounds

        // Ctrl+wheel zooms about the cursor; a plain wheel scrolls, as Flickable already does.
        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: function(event) {
                const factor = event.angleDelta.y > 0 ? 1.1 : 1 / 1.1
                const next = Math.max(root.minZoom, Math.min(root.maxZoom, canvas.scale * factor))
                if (next === canvas.scale)
                    return
                const cx = (flick.contentX + event.x) / canvas.scale
                const cy = (flick.contentY + event.y) / canvas.scale
                canvas.scale = next
                flick.contentX = cx * next - event.x
                flick.contentY = cy * next - event.y
            }
        }

        Item {
            id: canvas
            // The layout works in its own coordinates; shift them so the graph starts at the origin.
            width: Math.max(GraphViewModel.boundsWidth, flick.width)
            height: Math.max(GraphViewModel.boundsHeight, flick.height)
            transformOrigin: Item.TopLeft
            scale: 1.0

            Item {
                x: -GraphViewModel.boundsX
                y: -GraphViewModel.boundsY

                // Edges under nodes: an edge that ends at a box should disappear beneath it.
                Repeater {
                    model: GraphViewModel.edges

                    delegate: Shape {
                        required property string svgPath
                        required property string arrowSvgPath
                        required property color edgeColor
                        required property real edgeWidth
                        required property bool isDashed
                        required property bool isActive
                        required property var edgeId

                        anchors.fill: parent
                        // GPU-side curve rendering; the alternative tessellates every bezier on the
                        // CPU each time the zoom changes.
                        preferredRendererType: Shape.CurveRenderer

                        ShapePath {
                            strokeColor: parent.edgeColor
                            strokeWidth: parent.edgeWidth
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            joinStyle: ShapePath.RoundJoin
                            strokeStyle: parent.isDashed ? ShapePath.DashLine : ShapePath.SolidLine
                            PathSvg { path: parent.parent.svgPath }
                        }

                        ShapePath {
                            strokeColor: parent.edgeColor
                            strokeWidth: parent.edgeWidth
                            fillColor: "transparent"
                            capStyle: ShapePath.RoundCap
                            PathSvg { path: parent.parent.arrowSvgPath }
                        }
                    }
                }

                Repeater {
                    id: nodeRepeater
                    model: GraphViewModel.nodes

                    delegate: Item {
                        id: node

                        required property int nodeType
                        required property var tokenId
                        required property var ownerTokenId
                        required property string name
                        required property real nodeX
                        required property real nodeY
                        required property real nodeWidth
                        required property real nodeHeight
                        required property color fillColor
                        required property color borderColor
                        required property color textColor
                        required property int cornerRadius
                        required property int borderWidth
                        required property bool borderDashed
                        required property string fontFamily
                        required property int fontSize
                        required property bool fontBold
                        required property real textOffsetX
                        required property real textOffsetY
                        required property url iconSource
                        required property real iconOffsetX
                        required property real iconOffsetY
                        required property int iconSize
                        required property bool isActive
                        required property bool isExpanded
                        required property bool isInteractive
                        required property int invisibleSubNodeCount
                        required property int bundledNodeCount
                        required property bool hasMissingChildNodes

                        x: nodeX
                        y: nodeY
                        width: nodeWidth
                        height: nodeHeight

                        Rectangle {
                            anchors.fill: parent
                            radius: node.cornerRadius
                            color: node.fillColor
                            border.width: node.borderWidth
                            border.color: node.borderColor
                            // Qualifier and text nodes are labels, not boxes.
                            visible: node.nodeType !== root.typeText
                        }

                        Image {
                            visible: node.iconSource != ""
                            source: node.iconSource
                            x: node.iconOffsetX
                            y: node.iconOffsetY
                            sourceSize.width: node.iconSize
                            sourceSize.height: node.iconSize
                            width: node.iconSize
                            height: node.iconSize
                            smooth: true
                        }

                        Text {
                            // Access sections label themselves with an icon, and the expand toggle
                            // with its own count below.
                            visible: node.nodeType !== root.typeAccess && node.nodeType !== root.typeExpandToggle
                            text: node.name + (node.hasMissingChildNodes ? "..." : "")
                            x: node.textOffsetX
                            y: node.textOffsetY
                            color: node.textColor
                            font.family: node.fontFamily
                            font.pixelSize: node.fontSize
                            font.bold: node.fontBold
                            renderType: Text.QtRendering
                        }

                        Text {
                            visible: node.nodeType === root.typeExpandToggle
                            anchors.centerIn: parent
                            text: node.isExpanded ? "−" : (node.invisibleSubNodeCount > 0
                                                                ? "+" + node.invisibleSubNodeCount : "+")
                            color: node.textColor
                            font.family: node.fontFamily
                            font.pixelSize: node.fontSize
                            renderType: Text.QtRendering
                        }

                        // The count circle a bundle carries in its top-right corner.
                        Rectangle {
                            visible: node.bundledNodeCount > 0
                            width: countText.implicitWidth + 8
                            height: 16
                            radius: 8
                            x: parent.width - width / 2
                            y: -height / 2
                            color: node.borderColor

                            Text {
                                id: countText
                                anchors.centerIn: parent
                                text: node.bundledNodeCount
                                color: node.fillColor
                                font.pixelSize: 10
                                renderType: Text.QtRendering
                            }
                        }

                        TapHandler {
                            enabled: node.isInteractive && node.nodeType !== root.typeText
                                     && node.nodeType !== root.typeAccess
                            acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                            onSingleTapped: function(event) {
                                if (event.button === Qt.MiddleButton) {
                                    if (node.tokenId)
                                        GraphViewModel.openInNewTab(node.tokenId)
                                    return
                                }
                                switch (node.nodeType) {
                                case root.typeExpandToggle:
                                    // The toggle has no token of its own; the box it sits in does.
                                    if (node.ownerTokenId)
                                        GraphViewModel.toggleExpand(node.ownerTokenId, node.isExpanded)
                                    break
                                case root.typeBundle:
                                    GraphViewModel.splitBundle(node.tokenId)
                                    break
                                case root.typeGroup:
                                    GraphViewModel.activateGroup(node.tokenId, GraphViewModel.grouping)
                                    break
                                default:
                                    if (node.tokenId)
                                        GraphViewModel.activateNode(node.tokenId, node.isActive, false)
                                    break
                                }
                            }
                        }

                        HoverHandler {
                            enabled: node.isInteractive
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: GraphViewModel.empty
        text: qsTr("No symbol activated")
        color: Theme.textFaint
        font.pixelSize: Theme.fontSize
    }
}
