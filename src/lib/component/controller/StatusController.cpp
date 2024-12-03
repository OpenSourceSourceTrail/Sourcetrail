#include "StatusController.h"

#include "IApplicationSettings.hpp"
#include "StatusView.h"
#include "utility.h"

StatusController::StatusController() {
  m_statusFilter = IApplicationSettings::getInstanceRaw()->getStatusFilter();
}

StatusController::~StatusController() = default;

StatusView* StatusController::getView() const {
  return Controller::getView<StatusView>();
}

void StatusController::clear() {}

void StatusController::handleMessage(MessageClearStatusView* /*message*/) {
  m_status.clear();
  getView()->clear();
}

void StatusController::handleMessage(MessageShowStatus* /*message*/) {
  getView()->showDockWidget();
}

void StatusController::handleMessage(MessageStatus* message) {
  if(message->status().empty()) {
    return;
  }

  std::vector<Status> stati;

  for(const std::wstring& status : message->stati()) {
    stati.emplace_back(status, message->isError);
  }

  utility::append(m_status, stati);

  addStatus(stati);
}

void StatusController::handleMessage(MessageStatusFilterChanged* message) {
  m_statusFilter = message->statusFilter;

  getView()->clear();
  addStatus(m_status);

  auto* settings = IApplicationSettings::getInstanceRaw();
  settings->setStatusFilter(m_statusFilter);
  settings->save();
}

void StatusController::addStatus(const std::vector<Status>& statuses) {
  std::vector<Status> filteredStatus;

  for(const Status& status : statuses) {
    if((static_cast<std::uint8_t>(status.type) & static_cast<std::uint8_t>(m_statusFilter)) != 0) {
      filteredStatus.push_back(status);
    }
  }

  getView()->addStatus(filteredStatus);
}