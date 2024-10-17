#include "RecentItemModelTestSuite.hpp"

#include <memory>
#include <thread>

#include <QApplication>
#include <QObject>
#include <QTest>
#include <QTimer>

#include <gmock/gmock.h>

#define private public
#include "../../../lib/tests/mocks/MockedMessageQueue.hpp"
#include "RecentItemModel.hpp"
#undef private

namespace {

QModelIndex createModelIndex(int index, const QAbstractListModel* input) {
  struct HackedModelIndex final {
    int r, c;
    quintptr i;
    const QAbstractItemModel* m;
  };

  QModelIndex model;
  auto* hackedModelIndex = reinterpret_cast<HackedModelIndex*>(&model);
  hackedModelIndex->r = index;
  hackedModelIndex->c = 0;
  hackedModelIndex->i = 0;
  hackedModelIndex->m = input;

  return model;
}

}    // namespace

const std::vector<std::filesystem::path> RecentItemModelTestSuite::RecentItems = std::vector<std::filesystem::path>{
    std::filesystem::path{"data/RecentItemModelTestSuite/test00.srctrlprj"},
    std::filesystem::path{"data/RecentItemModelTestSuite/test01.srctrlprj"},
    std::filesystem::path{"data/RecentItemModelTestSuite/missing.srctrlprj"},
};

const QIcon RecentItemModelTestSuite::ProjectIcon = QIcon("://icon/empty_icon.png");

RecentItemModelTestSuite::RecentItemModelTestSuite() = default;

RecentItemModelTestSuite::~RecentItemModelTestSuite() = default;

void RecentItemModelTestSuite::initTestCase() {
  Q_INIT_RESOURCE(resources);
  IMessageQueue::setInstance(std::make_shared<MockedMessageQueue>());
}

void RecentItemModelTestSuite::init() {
  constexpr size_t MaxRecentProjects = 10;
  mRecentItemModel = std::make_unique<qt::element::model::RecentItemModel>(RecentItems, MaxRecentProjects);
}

void RecentItemModelTestSuite::indexMissingProject() {
  QModelIndex index = createModelIndex(2, mRecentItemModel.get());

  const auto title = mRecentItemModel->data(index);
  QVERIFY(title.isValid());
  QCOMPARE(title.type(), QVariant::Type::String);
  QCOMPARE(title, "missing");

  const auto icon = mRecentItemModel->data(index, Qt::DecorationRole);
  QVERIFY(icon.isValid());
  QCOMPARE(icon.type(), QVariant::Type::Icon);
  QCOMPARE(icon.value<QIcon>().name(), ProjectIcon.name());

  const auto toolTip = mRecentItemModel->data(index, Qt::ToolTipRole);
  QVERIFY(toolTip.isValid());
  QCOMPARE(toolTip.type(), QVariant::Type::String);
  QCOMPARE(toolTip, QString("\"%0\" is missed").arg(QString::fromStdWString(RecentItems[2].wstring())));

  const auto font = mRecentItemModel->data(index, Qt::FontRole);
  QVERIFY(font.isValid());
  QCOMPARE(font.type(), QVariant::Type::Font);
  QVERIFY(font.value<QFont>().strikeOut());
}

void RecentItemModelTestSuite::indexOutOfBoundary() {
  constexpr int OutOfBoundary = 6;
  QModelIndex index = createModelIndex(OutOfBoundary, mRecentItemModel.get());
  QVERIFY(index.isValid());

  const auto title = mRecentItemModel->data(index);
  QVERIFY(!title.isValid());
}

void RecentItemModelTestSuite::indexGoodCase() {
  QModelIndex index = createModelIndex(0, mRecentItemModel.get());

  const auto title = mRecentItemModel->data(index);
  QVERIFY(title.isValid());
  QCOMPARE(title, "test00");

  const auto icon = mRecentItemModel->data(index, Qt::DecorationRole);
  QVERIFY(icon.isValid());
  QCOMPARE(icon.type(), QVariant::Type::Icon);
  QCOMPARE(icon.value<QIcon>().name(), ProjectIcon.name());

  const auto toolTip = mRecentItemModel->data(index, Qt::ToolTipRole);
  QVERIFY(!toolTip.isValid());

  const auto font = mRecentItemModel->data(index, Qt::FontRole);
  QVERIFY(!font.isValid());
}

void RecentItemModelTestSuite::indexInvalid() {
  const auto inValid = mRecentItemModel->data({});
  QVERIFY(!inValid.isValid());
}

void RecentItemModelTestSuite::invalidClicked() {
  mRecentItemModel->clicked(QModelIndex{});
}

void RecentItemModelTestSuite::outOfBoundaryClicked() {
  constexpr int OutOfBoundary = 6;
  QModelIndex index = createModelIndex(OutOfBoundary, mRecentItemModel.get());
  mRecentItemModel->clicked(index);
}

void RecentItemModelTestSuite::goodCaseClicked() {
  QModelIndex index = createModelIndex(0, mRecentItemModel.get());
  mRecentItemModel->clicked(index);
}

void RecentItemModelTestSuite::missingClicked() {
  QModelIndex index = createModelIndex(2, mRecentItemModel.get());
  // Close the dialog
  std::thread th([]() {
    bool waitingForTimeOut = true;
    QWidget* dialog = nullptr;
    const auto t0 = std::chrono::steady_clock::now();
    do {
      dialog = qApp->activeWindow();
      if(dialog != nullptr) {
        dialog->close();
        break;
      }
      const auto t1 = std::chrono::steady_clock::now();
      waitingForTimeOut = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() < 500;
    } while(waitingForTimeOut);
    if(dialog == nullptr) {
      qDebug() << "Missing";
      qApp->exit();
    }
  });
  mRecentItemModel->clicked(index);
  th.join();
}

#ifndef D_WINDOWS
void RecentItemModelTestSuite::removeItemWhileEmpty() {
  // It will fail otherwise
  qt::element::model::RecentItemModel itemModel({}, 10);
  QVERIFY(itemModel.mRecentProjects.empty());
  itemModel.removeItem(0);
}
#endif

void RecentItemModelTestSuite::removeMissingIndex() {
  // It will fail otherwise
  mRecentItemModel->removeItem(10);
}

#ifndef D_WINDOWS
void RecentItemModelTestSuite::removeGoodCase() {
  mRecentItemModel->removeItem(0);
  QCOMPARE(2, mRecentItemModel->mRecentProjects.size());
}
#endif

void RecentItemModelTestSuite::rowCountGoodCase() {
  QCOMPARE(3, mRecentItemModel->rowCount({}));
}

void RecentItemModelTestSuite::rowCountEmpty() {
  // It will fail otherwise
  qt::element::model::RecentItemModel itemModel({}, 10);
  QCOMPARE(0, itemModel.rowCount({}));
}

void RecentItemModelTestSuite::cleanup() {}

void RecentItemModelTestSuite::cleanupTestCase() {
  IMessageQueue::setInstance(nullptr);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleMock(&argc, argv);
  QTEST_MAIN_IMPL(RecentItemModelTestSuite)
}