#pragma once
#include <memory>
#include <string>
#include <vector>

#include "component/view/DialogView.h"

class Bookmark;
class MessageBase;

namespace lib {

/**
 * Everything Application asks of the user interface, and nothing else.
 *
 * Application used to reach the GUI through MainView, which dragged the whole widget-shaped
 * View/ViewFactory/ComponentManager scaffolding along with it. This is the same set of calls with
 * the scaffolding removed: a front end implements these thirteen methods and owns its own view
 * layer however it likes. Passing nullptr for the shell is what makes an Application headless --
 * that is how the engine daemon and the CLI run.
 */
struct IAppShell {
  virtual ~IAppShell();

  IAppShell() = default;
  IAppShell(const IAppShell&) = delete;
  IAppShell(IAppShell&&) = delete;
  IAppShell& operator=(const IAppShell&) = delete;
  IAppShell& operator=(IAppShell&&) = delete;

  /** Called once, after Application exists and before the message loop starts. */
  virtual void setup() = 0;

  /** Called on destruction, before the window goes away. */
  virtual void saveLayout() = 0;

  /** Tears the views down without destroying the shell; a new project reuses it. */
  virtual void clear() = 0;

  [[nodiscard]] virtual std::shared_ptr<DialogView> getDialogView(DialogView::UseCase useCase) = 0;

  virtual void refreshViews() = 0;
  virtual void refreshUIState(bool isAfterIndexing) = 0;

  virtual void loadWindow(bool showStartWindow) = 0;
  virtual void hideStartScreen() = 0;
  virtual void activateWindow() = 0;
  virtual void setTitle(const std::wstring& title) = 0;

  virtual void updateRecentProjectMenu() = 0;
  virtual void updateHistoryMenu(std::shared_ptr<MessageBase> message) = 0;
  virtual void updateBookmarksMenu(const std::vector<std::shared_ptr<Bookmark>>& bookmarks) = 0;
};

}    // namespace lib
