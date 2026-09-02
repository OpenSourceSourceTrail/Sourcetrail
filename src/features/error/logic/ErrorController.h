#pragma once
// STL
#include <map>
// internal
#include "activation/logic/ActivationListener.h"
#include "error/messages/MessageErrorCountClear.h"
#include "error/messages/MessageErrorCountUpdate.h"
#include "error/messages/MessageErrorsAll.h"
#include "error/messages/MessageErrorsForFile.h"
#include "error/messages/MessageErrorsHelpMessage.h"
#include "error/messages/MessageShowError.h"
#include "MessageListener.h"
#include "type/indexing/MessageIndexingFinished.h"
#include "type/indexing/MessageIndexingStarted.h"
//
#include "component/controller/Controller.h"
#include "error/logic/ErrorView.h"

class StorageAccess;

class ErrorController final
    : public Controller
    , public ActivationListener
    , public MessageListener<MessageErrorCountClear>
    , public MessageListener<MessageErrorCountUpdate>
    , public MessageListener<MessageErrorsAll>
    , public MessageListener<MessageErrorsForFile>
    , public MessageListener<MessageErrorsHelpMessage>
    , public MessageListener<MessageIndexingFinished>
    , public MessageListener<MessageIndexingStarted>
    , public MessageListener<MessageShowError> {
public:
  explicit ErrorController(StorageAccess* pStorageAccess);

  ~ErrorController() override;

  void errorFilterChanged(const ErrorFilter& filter);

  void showError(Id errorId);

private:
  /** @name Handle Messages
   * @{
   */
  void handleActivation(const MessageActivateBase* pMessage) override;

  void handleMessage(MessageActivateErrors* pMessage) override;
  void handleMessage(MessageErrorCountClear* pMessage) override;
  void handleMessage(MessageErrorCountUpdate* pMessage) override;
  void handleMessage(MessageErrorsAll* pMessage) override;
  void handleMessage(MessageErrorsForFile* pMessage) override;
  void handleMessage(MessageErrorsHelpMessage* pMessage) override;
  void handleMessage(MessageIndexingFinished* pMessage) override;
  void handleMessage(MessageIndexingStarted* pMessage) override;
  void handleMessage(MessageShowError* pMessage) override;
  /**
   * @}
   */

  ErrorView* getView() const;

  void clear() override;

  bool showErrors(const ErrorFilter& filter, bool scrollTo);

  StorageAccess* m_storageAccess;

  size_t m_errorCount = 0;

  std::map<Id, bool> m_tabShowsErrors;
  std::map<Id, FilePath> m_tabActiveFilePath;

  bool m_newErrorsAdded = false;
};