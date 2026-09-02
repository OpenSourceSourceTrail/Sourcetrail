#ifndef QT_IDE_COMMUNICATION_CONTROLLER
#define QT_IDE_COMMUNICATION_CONTROLLER

#include <qobject.h>

#include "ide_communication/logic/IDECommunicationController.h"
#include "ide_communication/ui/QtTcpWrapper.h"
#include "qt/utility/QtThreadedFunctor.h"

class StorageAccess;

class QtIDECommunicationController : public IDECommunicationController {
public:
  QtIDECommunicationController(QObject* parent, StorageAccess* storageAccess);
  ~QtIDECommunicationController();

  virtual void startListening();
  virtual void stopListening();

  virtual bool isListening() const;

private:
  virtual void sendMessage(const std::wstring& message) const;

  QtTcpWrapper m_tcpWrapper;

  QtThreadedLambdaFunctor m_onQtThread;
};

#endif    // QT_IDE_COMMUNICATION_CONTROLLER
