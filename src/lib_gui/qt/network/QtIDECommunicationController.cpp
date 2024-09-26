#include "QtIDECommunicationController.h"

#include <functional>

#include "IApplicationSettings.hpp"

QtIDECommunicationController::QtIDECommunicationController(QObject* parent, StorageAccess* storageAccess)
    : IDECommunicationController(storageAccess), m_tcpWrapper(parent) {
  m_tcpWrapper.setReadCallback(std::bind(&QtIDECommunicationController::handleIncomingMessage, this, std::placeholders::_1));
}

QtIDECommunicationController::~QtIDECommunicationController() = default;

void QtIDECommunicationController::startListening() {
  m_onQtThread([=]() {
    IApplicationSettings* appSettings = IApplicationSettings::getInstanceRaw();
    m_tcpWrapper.setServerPort(static_cast<uint16_t>(appSettings->getSourcetrailPort()));
    m_tcpWrapper.setClientPort(static_cast<uint16_t>(appSettings->getPluginPort()));
    m_tcpWrapper.startListening();

    sendUpdatePing();
  });
}

void QtIDECommunicationController::stopListening() {
  m_onQtThread([=]() { m_tcpWrapper.stopListening(); });
}

bool QtIDECommunicationController::isListening() const {
  return m_tcpWrapper.isListening();
}

void QtIDECommunicationController::sendMessage(const std::wstring& message) const {
  m_tcpWrapper.sendMessage(message);
}
