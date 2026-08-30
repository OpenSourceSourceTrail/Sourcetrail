#include "ShellMessages.h"

#include "AppShell.h"
#include "utilityString.h"

ShellMessages::ShellMessages(AppShell* shell) : mShell(shell) {}

ShellMessages::~ShellMessages() = default;

void ShellMessages::handleMessage(MessageStatus* message) {
  if(message == nullptr || !message->showInStatusBar) {
    return;
  }

  mShell->reportStatus(QString::fromStdString(utility::encodeToUtf8(message->status())), message->isError);
}
