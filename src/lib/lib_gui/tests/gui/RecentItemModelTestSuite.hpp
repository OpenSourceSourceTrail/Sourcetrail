#pragma once
#include <filesystem>
#include <memory>

#include <QIcon>
#include <QObject>

namespace qt::element::model {
class RecentItemModel;
}

class RecentItemModelTestSuite : public QObject {
  Q_OBJECT
public:
  RecentItemModelTestSuite();
  ~RecentItemModelTestSuite() override;

  static const std::vector<std::filesystem::path> RecentItems;

  static const QIcon ProjectIcon;

  std::unique_ptr<qt::element::model::RecentItemModel> mRecentItemModel;

private slots:
  void initTestCase();

  void init();

  void indexMissingProject();

  void indexOutOfBoundary();

  void indexGoodCase();

  void indexInvalid();

  void invalidClicked();

  void outOfBoundaryClicked();

  void goodCaseClicked();

  void missingClicked();

  void removeItemWhileEmpty();

  void removeMissingIndex();

  void removeGoodCase();

  void rowCountGoodCase();

  void rowCountEmpty();

  void cleanup();

  void cleanupTestCase();
};
