#pragma once
#include <QObject>

class QtSelfRefreshIconButtonTestSuite : public QObject {
  Q_OBJECT
public:
  QtSelfRefreshIconButtonTestSuite();

  ~QtSelfRefreshIconButtonTestSuite() override;

private slots:
  void initTestCase();

  void constructWithEmptyString();

  void constructGoodCase();

  void setTextWithEmptyString();

  void setTextGoodCase();

  void setIconPath();

  void cleanupTestCase();

  void setIconPathWithEmptyString();
};