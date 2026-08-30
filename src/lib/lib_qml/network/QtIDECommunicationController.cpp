#include "network/QtIDECommunicationController.h"

#include "GuiThread.h"
#include "settings/IApplicationSettings.hpp"

QtIDECommunicationController::QtIDECommunicationController(QObject* parent, StorageAccess* storageAccess)
    : QObject(parent), IDECommunicationController(storageAccess), mTcpWrapper(this) {
  mTcpWrapper.setReadCallback([this](const std::wstring& message) { handleIncomingMessage(message); });
}

QtIDECommunicationController::~QtIDECommunicationController() = default;

void QtIDECommunicationController::startListening() {
  qml::postToGui(this, [this]() {
    IApplicationSettings* appSettings = IApplicationSettings::getInstanceRaw();
    mTcpWrapper.setServerPort(static_cast<uint16_t>(appSettings->getSourcetrailPort()));
    mTcpWrapper.setClientPort(static_cast<uint16_t>(appSettings->getPluginPort()));
    mTcpWrapper.startListening();

    sendUpdatePing();
  });
}

void QtIDECommunicationController::stopListening() {
  qml::postToGui(this, [this]() { mTcpWrapper.stopListening(); });
}

bool QtIDECommunicationController::isListening() const {
  return mTcpWrapper.isListening();
}

void QtIDECommunicationController::sendMessage(const std::wstring& message) const {
  mTcpWrapper.sendMessage(message);
}
