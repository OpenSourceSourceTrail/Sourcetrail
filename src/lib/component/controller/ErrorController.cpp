#include "ErrorController.h"
// STL
#include <vector>
// internal
#include "Application.h"
#include "DialogView.h"
#include "Project.h"
#include "QtHelpButtonInfo.h"
#include "StorageAccess.h"
#include "TabId.h"
#include "IApplicationSettings.hpp"

ErrorController::ErrorController(StorageAccess* pStorageAccess) : m_storageAccess(pStorageAccess) {}

ErrorController::~ErrorController() = default;

void ErrorController::errorFilterChanged(const ErrorFilter& filter) {
  if(m_tabActiveFilePath[TabId::currentTab()].empty()) {
    MessageActivateErrors(filter).dispatch();
    return;
  }
  MessageActivateErrors(filter, m_tabActiveFilePath[TabId::currentTab()]).dispatch();
}

void ErrorController::showError(Id errorId) {
  if(!m_tabShowsErrors[TabId::currentTab()] || m_newErrorsAdded) {
    errorFilterChanged(getView()->getErrorFilter());
  }

  MessageShowError(errorId).dispatch();
}

void ErrorController::handleActivation(const MessageActivateBase* pMessage) {
  m_tabShowsErrors[dynamic_cast<const MessageBase*>(pMessage)->getSchedulerId()] = false;
}

void ErrorController::handleMessage(MessageActivateErrors* pMessage) {
  clear();

  m_tabShowsErrors[pMessage->getSchedulerId()] = true;
  m_tabActiveFilePath[pMessage->getSchedulerId()] = pMessage->file;

  ErrorView* view = getView();
  view->setErrorFilter(pMessage->filter);

  if(showErrors(pMessage->filter, true)) {
    view->showDockWidget();
  }
}

void ErrorController::handleMessage(MessageErrorCountClear* /*pMessage*/) {
  clear();

  ErrorView* view = getView();

  ErrorFilter filter = view->getErrorFilter();
  filter.limit = ErrorFilter().limit;
  view->setErrorFilter(filter);
}

void ErrorController::handleMessage(MessageErrorCountUpdate* pMessage) {
  m_storageAccess->addErrorsToCache(pMessage->newErrors, pMessage->errorCount);
  m_newErrorsAdded = true;

  ErrorFilter filter = getView()->getErrorFilter();

  int room = static_cast<int>(filter.limit) - static_cast<int>(m_errorCount);
  if(room > 0) {
    filter.limit = 0;
    std::vector<ErrorInfo> errors;

    for(const ErrorInfo& error : pMessage->newErrors) {
      if(filter.filter(error)) {
        errors.push_back(error);

        if(room > 0 && errors.size() >= size_t(room)) {
          break;
        }
      }
    }

    getView()->addErrors(errors, pMessage->errorCount, true);

    if(!Application::getInstance()->getDialogView(DialogView::UseCase::INDEXING)->dialogsHidden()) {
      getView()->showDockWidget();
    }

    m_errorCount += errors.size();
  }
}

void ErrorController::handleMessage(MessageErrorsAll* /*pMessage*/) {
  MessageActivateErrors(getView()->getErrorFilter()).dispatch();
}

void ErrorController::handleMessage(MessageErrorsForFile* pMessage) {
  std::shared_ptr<const Project> project = Application::getInstance()->getCurrentProject();
  if(project && project->isIndexing()) {
    Application::getInstance()->handleDialog(L"Showing errors for a file is not possible while indexing.");
    return;
  }

  MessageActivateErrors(ErrorFilter(), pMessage->file).dispatch();
}

void ErrorController::handleMessage(MessageErrorsHelpMessage* pMessage) {
  IApplicationSettings* appSettings = IApplicationSettings::getInstanceRaw();
  if(!pMessage->force) {
    if(appSettings->getSeenErrorHelpMessage()) {
      return;
    }

    ErrorCountInfo info = m_storageAccess->getErrorCount();
    if(!info.total) {
      return;
    }
  }

  appSettings->setSeenErrorHelpMessage(true);
  appSettings->save();

  m_onQtThread([=]() { createErrorHelpButtonInfo().displayMessage(nullptr); });
}

void ErrorController::handleMessage(MessageIndexingFinished* /*pMessage*/) {
  m_storageAccess->setUseErrorCache(false);

  clear();

  showErrors(getView()->getErrorFilter(), false);
}

void ErrorController::handleMessage(MessageIndexingStarted* /*pMessage*/) {
  m_storageAccess->setUseErrorCache(true);
}

void ErrorController::handleMessage(MessageShowError* pMessage) {
  getView()->setErrorId(pMessage->errorId);
}

ErrorView* ErrorController::getView() const {
  return Controller::getView<ErrorView>();
}

void ErrorController::clear() {
  m_errorCount = 0;
  m_tabShowsErrors.clear();
  m_tabActiveFilePath.clear();
  m_newErrorsAdded = false;

  getView()->clear();
}

bool ErrorController::showErrors(const ErrorFilter& filter, bool scrollTo) {
  auto* pView = getView();

  ErrorFilter filterUnlimited = filter;
  filterUnlimited.limit = 0;

  std::vector<ErrorInfo> errors;
  if(m_tabActiveFilePath[TabId::currentTab()].empty()) {
    errors = m_storageAccess->getErrorsLimited(filterUnlimited);
  } else {
    errors = m_storageAccess->getErrorsForFileLimited(filter, m_tabActiveFilePath[TabId::currentTab()]);
  }

  ErrorCountInfo errorCount(errors);
  if(filter.limit > 0 && errors.size() > filter.limit) {
    errors.resize(filter.limit);
  }

  pView->addErrors(errors, errorCount, scrollTo);

  return !errors.empty();
}