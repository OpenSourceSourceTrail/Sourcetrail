#include "StatusController.h"

#include "IApplicationSettings.hpp"
#include "StatusView.h"
#include "utility.h"

StatusController::StatusController()
    : mStatusFilter(static_cast<std::uint8_t>(IApplicationSettings::getInstanceRaw()->getStatusFilter())) {}

StatusController::~StatusController() = default;

StatusView* StatusController::getView() const {
  return Controller::getView<StatusView>();
}

void StatusController::clear() {}

void StatusController::handleMessage(MessageClearStatusView* /*message*/) {
  mStatus.clear();
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

  utility::append(mStatus, stati);

  addStatus(stati);
}

void StatusController::handleMessage(MessageStatusFilterChanged* message) {
  mStatusFilter = message->statusFilter;

  getView()->clear();
  addStatus(mStatus);

  auto* settings = IApplicationSettings::getInstanceRaw();
  settings->setStatusFilter(mStatusFilter);
  settings->save();
}

void StatusController::addStatus(const std::vector<Status>& statuses) {
  std::vector<Status> filteredStatus;

  for(const Status& status : statuses) {
    if((static_cast<std::uint8_t>(status.type) & static_cast<std::uint8_t>(mStatusFilter)) != 0) {
      filteredStatus.push_back(status);
    }
  }

  getView()->addStatus(filteredStatus);
}