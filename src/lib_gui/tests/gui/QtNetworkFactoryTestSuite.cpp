#include "QtNetworkFactoryTestSuite.hpp"

#include <QTest>

#include "QtNetworkFactory.h"

void QtNetworkFactoryTestSuite::goodCase() {
  const QtNetworkFactory mFactory;
  QVERIFY(mFactory.createIDECommunicationController(nullptr) != nullptr);
}

QTEST_MAIN(QtNetworkFactoryTestSuite)