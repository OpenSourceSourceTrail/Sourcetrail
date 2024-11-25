#pragma once
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "FilePath.h"
#include "GlobalId.hpp"
#include "NodeKind.h"
#include "Tree.h"

class NodeType {
public:
  enum StyleType { STYLE_PACKAGE = 0, STYLE_SMALL_NODE = 1, STYLE_BIG_NODE = 2, STYLE_GROUP = 3 };

  struct BundleInfo {
    BundleInfo() = default;

    explicit BundleInfo(std::wstring bundleName_)
        : nameMatcher([](const std::wstring&) { return true; }), bundleName(std::move(bundleName_)) {}

    BundleInfo(std::function<bool(std::wstring)> nameMatcher_, std::wstring bundleName_)
        : nameMatcher(nameMatcher_), bundleName(std::move(bundleName_)) {}

    [[nodiscard]] bool isValid() const {
      return !bundleName.empty();
    }

    std::function<bool(const std::wstring&)> nameMatcher = nullptr;
    std::wstring bundleName;
  };

  static std::vector<NodeType> getOverviewBundleNodeTypesOrdered();

  explicit NodeType(NodeKind kind);

  bool operator==(const NodeType& other) const;
  bool operator!=(const NodeType& other) const;
  bool operator<(const NodeType& other) const;

  [[nodiscard]] NodeKind getKind() const;

  [[nodiscard]] Id getId() const;
  [[nodiscard]] bool isFile() const;
  [[nodiscard]] bool isBuiltin() const;
  [[nodiscard]] bool isUnknownSymbol() const;
  [[nodiscard]] bool isInheritable() const;
  [[nodiscard]] bool isPackage() const;
  [[nodiscard]] bool isCallable() const;
  [[nodiscard]] bool isVariable() const;
  [[nodiscard]] bool isUsable() const;
  [[nodiscard]] bool isPotentialMember() const;
  [[nodiscard]] bool isCollapsible() const;
  [[nodiscard]] bool isVisibleAsParentInGraph() const;
  [[nodiscard]] bool hasSearchFilter() const;
  [[nodiscard]] Tree<BundleInfo> getOverviewBundleTree() const;

  [[nodiscard]] FilePath getIconPath() const;

  [[nodiscard]] bool hasIcon() const;
  [[nodiscard]] StyleType getNodeStyle() const;

  [[nodiscard]] bool hasOverviewBundle() const;
  [[nodiscard]] std::string getUnderscoredTypeString() const;
  [[nodiscard]] std::string getReadableTypeString() const;
  [[nodiscard]] std::wstring getUnderscoredTypeWString() const;
  [[nodiscard]] std::wstring getReadableTypeWString() const;

  static std::vector<NodeType> const overviewBundleNodeTypesOrdered;

private:
  NodeKind m_kind;
};
