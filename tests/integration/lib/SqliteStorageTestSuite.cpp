#include <QDebug>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QVariant>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "SqliteStorage.h"
#include "TimeStamp.h"

struct MockedSqliteStorage : SqliteStorage {
  using SqliteStorage::SqliteStorage;
  MOCK_METHOD(size_t, getStaticVersion, (), (const, override));
  MOCK_METHOD(void, clearTables, (), (override));
  MOCK_METHOD(void, setupTables, (), (override));
  MOCK_METHOD(void, setupPrecompiledStatements, (), (override));
  using SqliteStorage::clearMetaTable;
  using SqliteStorage::executeStatement;
  using SqliteStorage::executeStatementScalar;
};

namespace {

using testing::Return;
using testing::StrictMock;
using testing::Test;

bool checkTableExists(const QString& dbFullPath, const QString& tableName) noexcept {
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(dbFullPath);
  if(!db.open()) {
    return false;
  }

  const auto tables = db.tables();
  return tables.contains(tableName);
}

TEST(SqliteStorage, setup_GoodCase) {
  // Given:
  const FilePath dbFullPath{"/tmp/setup_GoodCase.sqlite"};
  MockedSqliteStorage sqliteStorage{dbFullPath};

  // And:
  EXPECT_CALL(sqliteStorage, setupTables);
  EXPECT_CALL(sqliteStorage, setupPrecompiledStatements);

  // When:
  ASSERT_NO_THROW(sqliteStorage.setup());

  // Then:
  EXPECT_TRUE(checkTableExists(QString::fromStdString(dbFullPath.str()), "meta"));
}

TEST(SqliteStorage, getVersion_EmptyVersion) {
  // Given:
  MockedSqliteStorage sqliteStorage;

  // And:
  EXPECT_CALL(sqliteStorage, setupTables);
  EXPECT_CALL(sqliteStorage, setupPrecompiledStatements);
  ASSERT_NO_THROW(sqliteStorage.setup());

  // When:
  const auto version = sqliteStorage.getVersion();

  // Then:
  EXPECT_EQ(0, version);
}

TEST(SqliteStorage, getVersion_SetVersion) {
  // Given:
  MockedSqliteStorage sqliteStorage;

  // And:
  EXPECT_CALL(sqliteStorage, setupTables);
  EXPECT_CALL(sqliteStorage, setupPrecompiledStatements);
  ASSERT_NO_THROW(sqliteStorage.setup());
  sqliteStorage.setVersion(100);

  // When:
  const auto version = sqliteStorage.getVersion();

  // Then:
  EXPECT_EQ(100, version);
}

TEST(SqliteStorage, setVersion_InvaludVersion) {
  // Given:
  MockedSqliteStorage sqliteStorage;

  // And:
  EXPECT_CALL(sqliteStorage, setupTables);
  EXPECT_CALL(sqliteStorage, setupPrecompiledStatements);
  ASSERT_NO_THROW(sqliteStorage.setup());

  // When:
  sqliteStorage.setVersion(static_cast<size_t>(-1));

  // Then:
  EXPECT_THROW(sqliteStorage.getVersion(), std::out_of_range);
}

TEST(SqliteStorage, executeStatementScalar_InvalidQuery) {
  // Given:
  MockedSqliteStorage sqliteStorage;

  // When:
  const auto value = sqliteStorage.executeStatementScalar("invalid query", 0);

  // Then:
  EXPECT_EQ(0, value);
}

struct SqliteStorageFix : Test {
  void SetUp() override {
    mSqliteStorage = std::make_unique<StrictMock<MockedSqliteStorage>>();

    EXPECT_CALL(*mSqliteStorage, setupTables).WillOnce(Return());
    EXPECT_CALL(*mSqliteStorage, setupPrecompiledStatements).WillOnce(Return());

    ASSERT_NO_THROW(mSqliteStorage->setup());
  }

  void TearDown() override {
    mSqliteStorage.reset();
  }

  std::unique_ptr<testing::StrictMock<MockedSqliteStorage>> mSqliteStorage;
};

TEST_F(SqliteStorageFix, clear) {
  // Given:
  EXPECT_CALL(*mSqliteStorage, clearTables).WillOnce(Return());
  EXPECT_CALL(*mSqliteStorage, setupTables).WillOnce(Return());
  // When:
  mSqliteStorage->clear();
  // Then:
  EXPECT_TRUE(mSqliteStorage->isEmpty());
  EXPECT_EQ(0, mSqliteStorage->getVersion());
}

TEST_F(SqliteStorageFix, getDbFilePath) {
  const auto emptyFile = mSqliteStorage->getDbFilePath();
  EXPECT_TRUE(emptyFile.empty());
}

TEST_F(SqliteStorageFix, isEmpty) {
  EXPECT_TRUE(mSqliteStorage->isEmpty());
}

TEST_F(SqliteStorageFix, isIncompatible) {
  EXPECT_TRUE(mSqliteStorage->isIncompatible());
}

TEST_F(SqliteStorageFix, getTime) {
  ASSERT_FALSE(mSqliteStorage->getTime().isValid());
}

TEST_F(SqliteStorageFix, setTime) {
  ASSERT_NO_THROW(mSqliteStorage->setTime());
  const auto time = mSqliteStorage->getTime();
  ASSERT_TRUE(time.isValid());
}

TEST_F(SqliteStorageFix, executeStatementScalar) {
  // Given:
  ASSERT_TRUE(mSqliteStorage);

  // When:
  const auto value = mSqliteStorage->executeStatementScalar("SELECT count(*) FROM sqlite_master WHERE type = 'table';", 100);

  // Then:
  EXPECT_EQ(1, value);
}

TEST_F(SqliteStorageFix, executeStatement_InvaludQuery) {
  // Given:
  ASSERT_TRUE(mSqliteStorage);

  // When:
  ASSERT_FALSE(mSqliteStorage->executeStatement("INVALID_QUERY"));
}

TEST_F(SqliteStorageFix, executeStatement) {
  // Given:
  ASSERT_TRUE(mSqliteStorage);

  // When:
  ASSERT_TRUE(
      mSqliteStorage->executeStatement("CREATE TABLE IF NOT EXISTS test("
                                       "id INTEGER, "
                                       "PRIMARY KEY(id)"
                                       ");"));

  // And:
  const auto value = mSqliteStorage->executeStatementScalar("SELECT count(*) FROM sqlite_master WHERE type = 'table';", 100);

  // Then:
  EXPECT_EQ(2, value);
}

}    // namespace
