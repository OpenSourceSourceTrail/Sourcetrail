#pragma once
#include "ActivationListener.h"
#include "Controller.h"
#include "MessageListener.h"
#include "type/search/MessageFind.h"
#include "type/search/MessageSearchAutocomplete.h"

class StorageAccess;
class SearchView;

class SearchController
    : public Controller
    , public ActivationListener
    , public MessageListener<MessageFind>
    , public MessageListener<MessageSearchAutocomplete> {
public:
  explicit SearchController(StorageAccess* storageAccess);
  ~SearchController() override;

  [[nodiscard]] Id getSchedulerId() const override;

private:
  void handleActivation(const MessageActivateBase* message) override;

  void handleMessage(MessageFind* message) override;
  void handleMessage(MessageSearchAutocomplete* message) override;

  [[nodiscard]] SearchView* getView() const;

  void clear() override;

  void updateMatches(const MessageActivateBase* message, bool updateView = true);

  StorageAccess* mStorageAccess;
};
