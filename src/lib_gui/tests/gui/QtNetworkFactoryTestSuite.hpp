#pragma once
#include <QObject>

class QtNetworkFactoryTestSuite : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void goodCase();
  void cleanup();
};
