#pragma once
#include <QObject>

#include "component/controller/IDECommunicationController.h"
#include "network/QtTcpWrapper.h"

class StorageAccess;

/**
 * The IDE plugin protocol, unchanged from the widget GUI apart from how it reaches the GUI thread.
 *
 * QTcpServer must be driven from the thread its QObject lives on, but the messages that ask this
 * controller to start or stop arrive on the bus thread, hence the hop through qml::postToGui.
 */
class QtIDECommunicationController final
    : public QObject
    , public IDECommunicationController {
  Q_OBJECT

public:
  QtIDECommunicationController(QObject* parent, StorageAccess* storageAccess);
  ~QtIDECommunicationController() override;

  void startListening() override;
  void stopListening() override;

  [[nodiscard]] bool isListening() const override;

private:
  void sendMessage(const std::wstring& message) const override;

  QtTcpWrapper mTcpWrapper;
};
