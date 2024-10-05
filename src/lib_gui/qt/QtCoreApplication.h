#pragma once
// Qt5
#include <QCoreApplication>
// internal
#include "type/indexing/MessageIndexingStatus.h"
#include "MessageListener.h"
#include "type/MessageQuitApplication.h"
#include "type/MessageStatus.h"

class QtCoreApplication
    : public QCoreApplication
    , public MessageListener<MessageQuitApplication>
    , public MessageListener<MessageIndexingStatus>
    , public MessageListener<MessageStatus> {
  // NOLINTNEXTLINE(readability-identifier-length)
  Q_OBJECT

public:
  QtCoreApplication(int argc, char** argv);

  ~QtCoreApplication() override;

private:
  void handleMessage(MessageQuitApplication* pMessage) override;

  void handleMessage(MessageIndexingStatus* pMessage) override;

  void handleMessage(MessageStatus* pMessage) override;
};
