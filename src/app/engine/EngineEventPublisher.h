#pragma once
#include "component/view/DialogView.h"
#include "error/messages/MessageErrorCountUpdate.h"
#include "MessageListener.h"
#include "type/indexing/MessageIndexingStarted.h"
#include "type/MessageStatus.h"

class EngineHttpService;

/**
 * The engine's half of the event stream.
 *
 * Everything that reports progress -- Project, the index task graph, TaskBuildIndex -- talks to the
 * engine's own message bus and to a DialogView, both of which are process-local. The client is in
 * another process and hears none of it, which is why indexing used to sit at 0%. These two classes
 * are the only things that turn that local chatter into EngineEvents on the wire.
 *
 * EngineDialogView carries the file counts; the bus carries everything else. The split is not a
 * design choice, it is where the data happens to live: MessageIndexingStatus has room for a
 * percentage and nothing more.
 */
class EngineDialogView final : public DialogView {
public:
  EngineDialogView(UseCase useCase, EngineHttpService* service);

  void clearDialogs() override;

  void showUnknownProgressDialog(const std::wstring& title, const std::wstring& message) override;
  void hideUnknownProgressDialog() override;

  void showProgressDialog(const std::wstring& title, const std::wstring& message, size_t progress) override;
  void hideProgressDialog() override;

  void doUpdateIndexingDialog(size_t startedFileCount,
                              size_t finishedFileCount,
                              size_t totalFileCount,
                              const std::vector<FilePath>& sourcePaths) override;
  void updateCustomIndexingDialog(size_t startedFileCount,
                                  size_t finishedFileCount,
                                  size_t totalFileCount,
                                  const std::vector<FilePath>& sourcePaths) override;

  DatabasePolicy finishedIndexingDialog(size_t indexedFileCount,
                                        size_t totalIndexedFileCount,
                                        size_t completedFileCount,
                                        size_t totalFileCount,
                                        float time,
                                        ErrorCountInfo errorInfo,
                                        bool interrupted,
                                        bool shallow) override;

private:
  EngineHttpService* mService;
};

class EngineEventPublisher final
    : public MessageListener<MessageIndexingStarted>
    , public MessageListener<MessageStatus>
    , public MessageListener<MessageErrorCountUpdate> {
public:
  explicit EngineEventPublisher(EngineHttpService* service);

  /** Installs itself as Application's DialogView source. Call once, after createInstance. */
  void installDialogViewFactory() const;

private:
  void handleMessage(MessageIndexingStarted* message) override;
  void handleMessage(MessageStatus* message) override;
  void handleMessage(MessageErrorCountUpdate* message) override;

  EngineHttpService* mService;
};
