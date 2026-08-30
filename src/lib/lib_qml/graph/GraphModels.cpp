#include "graph/GraphModels.h"

namespace graph {

NodeModel::NodeModel(QObject* parent) : QAbstractListModel(parent) {}

int NodeModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(mNodes.size());
}

QVariant NodeModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(mNodes.size())) {
    return {};
  }

  const NodeItem& node = mNodes[static_cast<size_t>(index.row())];
  switch(role) {
  case TypeRole:
    return node.type;
  case TokenIdRole:
    return QVariant::fromValue(static_cast<qulonglong>(node.tokenId));
  case OwnerTokenIdRole:
    return QVariant::fromValue(static_cast<qulonglong>(node.ownerTokenId));
  case NameRole:
    return node.name;
  case XRole:
    return node.x;
  case YRole:
    return node.y;
  case WidthRole:
    return node.width;
  case HeightRole:
    return node.height;
  case FillColorRole:
    return node.fillColor;
  case BorderColorRole:
    return node.borderColor;
  case TextColorRole:
    return node.textColor;
  case CornerRadiusRole:
    return node.cornerRadius;
  case BorderWidthRole:
    return node.borderWidth;
  case BorderDashedRole:
    return node.borderDashed;
  case FontFamilyRole:
    return node.fontFamily;
  case FontSizeRole:
    return node.fontSize;
  case FontBoldRole:
    return node.fontBold;
  case TextOffsetXRole:
    return node.textOffsetX;
  case TextOffsetYRole:
    return node.textOffsetY;
  case IconSourceRole:
    return node.iconSource;
  case IconOffsetXRole:
    return node.iconOffsetX;
  case IconOffsetYRole:
    return node.iconOffsetY;
  case IconSizeRole:
    return node.iconSize;
  case ActiveRole:
    return node.active;
  case ConnectedRole:
    return node.connected;
  case ExpandedRole:
    return node.expanded;
  case InteractiveRole:
    return node.interactive;
  case HasHatchingRole:
    return node.hasHatching;
  case InvisibleSubNodeCountRole:
    return node.invisibleSubNodeCount;
  case BundledNodeCountRole:
    return node.bundledNodeCount;
  case HasMissingChildNodesRole:
    return node.hasMissingChildNodes;
  default:
    return {};
  }
}

QHash<int, QByteArray> NodeModel::roleNames() const {
  return {
      {TypeRole, "nodeType"},
      {TokenIdRole, "tokenId"},
      {OwnerTokenIdRole, "ownerTokenId"},
      {NameRole, "name"},
      {XRole, "nodeX"},
      {YRole, "nodeY"},
      {WidthRole, "nodeWidth"},
      {HeightRole, "nodeHeight"},
      {FillColorRole, "fillColor"},
      {BorderColorRole, "borderColor"},
      {TextColorRole, "textColor"},
      {CornerRadiusRole, "cornerRadius"},
      {BorderWidthRole, "borderWidth"},
      {BorderDashedRole, "borderDashed"},
      {FontFamilyRole, "fontFamily"},
      {FontSizeRole, "fontSize"},
      {FontBoldRole, "fontBold"},
      {TextOffsetXRole, "textOffsetX"},
      {TextOffsetYRole, "textOffsetY"},
      {IconSourceRole, "iconSource"},
      {IconOffsetXRole, "iconOffsetX"},
      {IconOffsetYRole, "iconOffsetY"},
      {IconSizeRole, "iconSize"},
      {ActiveRole, "isActive"},
      {ConnectedRole, "isConnected"},
      {ExpandedRole, "isExpanded"},
      {InteractiveRole, "isInteractive"},
      {HasHatchingRole, "hasHatching"},
      {InvisibleSubNodeCountRole, "invisibleSubNodeCount"},
      {BundledNodeCountRole, "bundledNodeCount"},
      {HasMissingChildNodesRole, "hasMissingChildNodes"},
  };
}

void NodeModel::reset(std::vector<NodeItem> nodes) {
  beginResetModel();
  mNodes = std::move(nodes);
  endResetModel();
}

EdgeModel::EdgeModel(QObject* parent) : QAbstractListModel(parent) {}

int EdgeModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(mEdges.size());
}

QVariant EdgeModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(mEdges.size())) {
    return {};
  }

  const EdgeItem& edge = mEdges[static_cast<size_t>(index.row())];
  switch(role) {
  case IdRole:
    return QVariant::fromValue(static_cast<qulonglong>(edge.id));
  case TokenIdRole:
    return QVariant::fromValue(static_cast<qulonglong>(edge.tokenId));
  case PathRole:
    return edge.path;
  case ArrowPathRole:
    return edge.arrowPath;
  case ColorRole:
    return edge.color;
  case WidthRole:
    return edge.width;
  case DashedRole:
    return edge.dashed;
  case ActiveRole:
    return edge.active;
  default:
    return {};
  }
}

QHash<int, QByteArray> EdgeModel::roleNames() const {
  return {
      {IdRole, "edgeId"},
      {TokenIdRole, "tokenId"},
      {PathRole, "svgPath"},
      {ArrowPathRole, "arrowSvgPath"},
      {ColorRole, "edgeColor"},
      {WidthRole, "edgeWidth"},
      {DashedRole, "isDashed"},
      {ActiveRole, "isActive"},
  };
}

void EdgeModel::reset(std::vector<EdgeItem> edges) {
  beginResetModel();
  mEdges = std::move(edges);
  endResetModel();
}

}    // namespace graph
