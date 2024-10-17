#include <gtest/gtest.h>

#include "logging.h"
#include "MatrixDynamicBase.h"

namespace {
std::vector<std::vector<int>> getTestValues(uint32_t numColumns, uint32_t numRows) {
  std::vector<std::vector<int>> testValues;

  for(unsigned int x = 0; x < numColumns; x++) {
    std::vector<int> row;

    for(unsigned int y = 0; y < numRows; y++) {
      row.push_back(x + y);
    }

    testValues.push_back(row);
  }

  return testValues;
}
}    // namespace

TEST(MatrixDynamicBase, constructors) {
  MatrixDynamicBase<int> matrix0;
  MatrixDynamicBase<int> matrix1(3, 5);

  std::vector<std::vector<int>> testValues = getTestValues(3, 5);
  MatrixDynamicBase<int> matrix2(testValues);

  EXPECT_TRUE(0 == matrix0.getColumnsCount());
  EXPECT_TRUE(0 == matrix0.getRowsCount());
  EXPECT_TRUE(3 == matrix1.getColumnsCount());
  EXPECT_TRUE(5 == matrix1.getRowsCount());
  EXPECT_TRUE(3 == matrix2.getColumnsCount());
  EXPECT_TRUE(5 == matrix2.getRowsCount());
}

TEST(MatrixDynamicBase, getValueAndSetValue) {
  std::vector<std::vector<int>> testValues = getTestValues(3, 5);
  MatrixDynamicBase<int> matrix0(testValues);

  EXPECT_TRUE(0 == matrix0.getValue(0, 0));
  EXPECT_TRUE(4 == matrix0.getValue(2, 2));
  EXPECT_TRUE(6 == matrix0.getValue(2, 4));

  matrix0.setValue(0, 0, 42);
  matrix0.setValue(2, 2, 84);
  matrix0.setValue(2, 4, 126);

  EXPECT_TRUE(42 == matrix0.getValue(0, 0));
  EXPECT_TRUE(84 == matrix0.getValue(2, 2));
  EXPECT_TRUE(126 == matrix0.getValue(2, 4));
}
