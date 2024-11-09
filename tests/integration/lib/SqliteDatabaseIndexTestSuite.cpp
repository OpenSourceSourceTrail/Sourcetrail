#include <gtest/gtest.h>

#include <CppSQLite3.h>

#include "SqliteDatabaseIndex.h"

namespace {

TEST(SqliteDatabaseIndexTestSuite, getName) {
  SqliteDatabaseIndex index("test_index", "test_target");
  EXPECT_EQ(index.getName(), "test_index");
}

TEST(SqliteDatabaseIndexTestSuite, createOnDatabase) {
  SqliteDatabaseIndex index("edge_source_node_id_index", "edge(source_node_id)");

  CppSQLite3DB database;
  ASSERT_NO_THROW(database.open(":memory:"));

  EXPECT_FALSE(index.createOnDatabase(database));
}

TEST(SqliteDatabaseIndexTestSuite, removeFromDatabase_WhileEmpty) {
  SqliteDatabaseIndex index("edge_source_node_id_index", "edge(source_node_id)");

  CppSQLite3DB database;
  ASSERT_NO_THROW(database.open(":memory:"));

  EXPECT_TRUE(index.removeFromDatabase(database));
}

TEST(SqliteDatabaseIndexTestSuite, removeFromDatabase_InvalidIndexName) {
  SqliteDatabaseIndex index("", "");

  CppSQLite3DB database;
  ASSERT_NO_THROW(database.open(":memory:"));

  EXPECT_FALSE(index.removeFromDatabase(database));
}

}    // namespace
