#ifndef UNDO_REDO_CONTROLLER_H
#define UNDO_REDO_CONTROLLER_H

#include <list>

#include "Controller.h"
#include "type/activation/MessageActivateErrors.h"
#include "type/activation/MessageActivateFullTextSearch.h"
#include "type/activation/MessageActivateLegend.h"
#include "type/code/MessageActivateLocalSymbols.h"
#include "type/activation/MessageActivateOverview.h"
#include "type/activation/MessageActivateTokens.h"
#include "type/activation/MessageActivateTrail.h"
#include "type/graph//MessageActivateTrailEdge.h"
#include "MessageBase.h"
#include "type/code/MessageChangeFileView.h"
#include "type/code/MessageCodeShowDefinition.h"
#include "type/graph/MessageDeactivateEdge.h"
#include "type/focus/MessageFocusChanged.h"
#include "type/graph/MessageGraphNodeBundleSplit.h"
#include "type/graph/MessageGraphNodeExpand.h"
#include "type/graph/MessageGraphNodeHide.h"
#include "type/graph/MessageGraphNodeMove.h"
#include "type/history/MessageHistoryRedo.h"
#include "type/history/MessageHistoryToPosition.h"
#include "type/history/MessageHistoryUndo.h"
#include "type/indexing//MessageIndexingFinished.h"
#include "MessageListener.h"
#include "type/MessageRefreshUIState.h"
#include "type/code/MessageScrollCode.h"
#include "type/graph/MessageScrollGraph.h"
#include "type/error/MessageShowError.h"
#include "type/code/MessageShowReference.h"
#include "type/code/MessageShowScope.h"

class StorageAccess;
class UndoRedoView;

class UndoRedoController
    : public Controller
    , public MessageListener<MessageActivateErrors>
    , public MessageListener<MessageActivateFullTextSearch>
    , public MessageListener<MessageActivateLegend>
    , public MessageListener<MessageActivateLocalSymbols>
    , public MessageListener<MessageActivateOverview>
    , public MessageListener<MessageActivateTokens>
    , public MessageListener<MessageActivateTrail>
    , public MessageListener<MessageActivateTrailEdge>
    , public MessageListener<MessageChangeFileView>
    , public MessageListener<MessageCodeShowDefinition>
    , public MessageListener<MessageDeactivateEdge>
    , public MessageListener<MessageFocusChanged>
    , public MessageListener<MessageGraphNodeBundleSplit>
    , public MessageListener<MessageGraphNodeExpand>
    , public MessageListener<MessageGraphNodeHide>
    , public MessageListener<MessageGraphNodeMove>
    , public MessageListener<MessageHistoryRedo>
    , public MessageListener<MessageHistoryToPosition>
    , public MessageListener<MessageHistoryUndo>
    , public MessageListener<MessageIndexingFinished>
    , public MessageListener<MessageRefreshUIState>
    , public MessageListener<MessageScrollCode>
    , public MessageListener<MessageScrollGraph>
    , public MessageListener<MessageShowError>
    , public MessageListener<MessageShowReference>
    , public MessageListener<MessageShowScope> {
public:
  UndoRedoController(StorageAccess* storageAccess);
  virtual ~UndoRedoController() = default;

  Id getSchedulerId() const override;

  UndoRedoView* getView();

  void clear() override;

private:
  struct Command {
    enum Order { ORDER_ACTIVATE, ORDER_ADAPT, ORDER_VIEW };

    Command(std::shared_ptr<MessageBase> message, Order order, bool replayLastOnly = false);

    std::shared_ptr<MessageBase> message;
    Order order;
    bool replayLastOnly;
  };

  void handleMessage(MessageActivateErrors* message) override;
  void handleMessage(MessageActivateFullTextSearch* message) override;
  void handleMessage(MessageActivateLegend* message) override;
  void handleMessage(MessageActivateLocalSymbols* message) override;
  void handleMessage(MessageActivateOverview* message) override;
  void handleMessage(MessageActivateTokens* message) override;
  void handleMessage(MessageActivateTrail* message) override;
  void handleMessage(MessageActivateTrailEdge* message) override;
  void handleMessage(MessageChangeFileView* message) override;
  void handleMessage(MessageCodeShowDefinition* message) override;
  void handleMessage(MessageDeactivateEdge* message) override;
  void handleMessage(MessageFocusChanged* message) override;
  void handleMessage(MessageGraphNodeBundleSplit* message) override;
  void handleMessage(MessageGraphNodeExpand* message) override;
  void handleMessage(MessageGraphNodeHide* message) override;
  void handleMessage(MessageGraphNodeMove* message) override;
  void handleMessage(MessageHistoryRedo* message) override;
  void handleMessage(MessageHistoryToPosition* message) override;
  void handleMessage(MessageHistoryUndo* message) override;
  void handleMessage(MessageIndexingFinished* message) override;
  void handleMessage(MessageRefreshUIState* message) override;
  void handleMessage(MessageScrollCode* message) override;
  void handleMessage(MessageScrollGraph* message) override;
  void handleMessage(MessageShowError* message) override;
  void handleMessage(MessageShowReference* message) override;
  void handleMessage(MessageShowScope* message) override;

  void replayCommands();
  void replayCommands(std::list<Command>::iterator it);
  void replayCommand(std::list<Command>::iterator it);

  void processCommand(Command command);

  bool sameMessageTypeAsLast(MessageBase* message) const;
  MessageBase* lastMessage() const;

  void updateHistoryMenu(std::shared_ptr<MessageBase> message);
  void updateHistory();

  void dump() const;

  StorageAccess* m_storageAccess;

  std::list<Command> m_list;
  std::list<Command>::iterator m_iterator;

  std::vector<std::shared_ptr<MessageBase>> m_history;
  size_t m_historyOffset;
};

#endif    // UNDO_REDO_CONTROLLER_H
