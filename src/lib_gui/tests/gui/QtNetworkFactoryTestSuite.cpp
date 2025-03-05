#include "QtNetworkFactoryTestSuite.hpp"

#include <QTest>

#include <gmock/gmock.h>

#include "../../../lib/tests/mocks/MockedMessageQueue.hpp"
#include "MessageQueue.h"
#include "QtNetworkFactory.h"

void QtNetworkFactoryTestSuite::initTestCase() {
  testing::InitGoogleMock();
}

void QtNetworkFactoryTestSuite::init() {
  IMessageQueue::setInstance(std::make_shared<MockedMessageQueue>());
}

void QtNetworkFactoryTestSuite::goodCase() {
  const QtNetworkFactory mFactory;
  QVERIFY(mFactory.createIDECommunicationController(nullptr) != nullptr);
}

void QtNetworkFactoryTestSuite::cleanup() {
  IMessageQueue::setInstance(nullptr);
}

QTEST_MAIN(QtNetworkFactoryTestSuite)