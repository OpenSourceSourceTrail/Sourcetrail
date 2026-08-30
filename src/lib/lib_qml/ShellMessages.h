#pragma once
#include "MessageListener.h"
#include "type/MessageStatus.h"

class AppShell;

/**
 * The shell's ear on the message bus.
 *
 * Separate from AppShell because MessageListenerBase registers itself in its constructor, and the
 * queue does not exist until Application::createInstance runs -- which is *after* main() has built
 * the shell. So this is created from AppShell::setup(), the one call Application makes once the bus
 * is up.
 */
class ShellMessages final : public MessageListener<MessageStatus> {
public:
  explicit ShellMessages(AppShell* shell);
  ~ShellMessages() override;

private:
  void handleMessage(MessageStatus* message) override;

  AppShell* mShell;
};
