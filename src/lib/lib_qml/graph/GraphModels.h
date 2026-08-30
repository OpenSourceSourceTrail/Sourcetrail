#pragma once
#include <vector>

#include <QAbstractListModel>

#include "graph/GraphSnapshot.h"

namespace graph {

/**
 * The painted nodes, as a model QML instantiates delegates from.
 *
 * Deliberately a *flat* list in paint order rather than a tree: the layout has already resolved
 * every nesting question into an absolute rectangle, so a tree would only cost QML a second
 * traversal and make viewport culling recursive.
 */
class NodeModel final : public QAbstractListModel {
  Q_OBJECT

public:
  enum Role {
    TypeRole = Qt::UserRole + 1,
    TokenIdRole,
    OwnerTokenIdRole,
    NameRole,
    XRole,
    YRole,
    WidthRole,
    HeightRole,
    FillColorRole,
    BorderColorRole,
    TextColorRole,
    CornerRadiusRole,
    BorderWidthRole,
    BorderDashedRole,
    FontFamilyRole,
    FontSizeRole,
    FontBoldRole,
    TextOffsetXRole,
    TextOffsetYRole,
    IconSourceRole,
    IconOffsetXRole,
    IconOffsetYRole,
    IconSizeRole,
    ActiveRole,
    ConnectedRole,
    ExpandedRole,
    InteractiveRole,
    HasHatchingRole,
    InvisibleSubNodeCountRole,
    BundledNodeCountRole,
    HasMissingChildNodesRole,
  };

  explicit NodeModel(QObject* parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  void reset(std::vector<NodeItem> nodes);

  [[nodiscard]] const std::vector<NodeItem>& items() const {
    return mNodes;
  }

private:
  std::vector<NodeItem> mNodes;
};

/** The painted edges. Each carries a ready-made SVG path in the same coordinates as the nodes. */
class EdgeModel final : public QAbstractListModel {
  Q_OBJECT

public:
  enum Role {
    IdRole = Qt::UserRole + 1,
    TokenIdRole,
    PathRole,
    ArrowPathRole,
    ColorRole,
    WidthRole,
    DashedRole,
    ActiveRole,
  };

  explicit EdgeModel(QObject* parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  void reset(std::vector<EdgeItem> edges);

private:
  std::vector<EdgeItem> mEdges;
};

}    // namespace graph
