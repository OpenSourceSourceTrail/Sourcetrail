#pragma once
#include <memory>

#include <QObject>

#include "../../../lib/tests/mocks/MockedApplicationSetting.hpp"

namespace qt::window {
class QtStartScreen;
}

class QtStartScreenTestSuite : public QObject {
  Q_OBJECT
public:
  QtStartScreenTestSuite();

  ~QtStartScreenTestSuite() override;

  std::unique_ptr<qt::window::QtStartScreen> mScreen;
  std::shared_ptr<MockedApplicationSettings> mMocked = std::make_shared<MockedApplicationSettings>();
private slots:
  void initTestCase();

  void init();

  void goodCase();

  void cleanup();
};