#pragma once
#include <QObject>
#include <QString>

// TODO(Hussein): Remove it
#include "ChatView.hpp"
#include "Controller.h"

class ChatController
    : public QObject
    , public Controller {
  Q_OBJECT
public:
  explicit ChatController() noexcept;
  Q_DISABLE_COPY_MOVE(ChatController)
  ~ChatController() noexcept override;

  void clear() override {}

  void sendMessage(const QString& message);

signals:
  void messageToAdd(const QString& message, Role role);
  void inputStateChanged(bool enabled);
  void clearInputRequested();
  void errorOccurred(const QString& error);

public slots:
  void onResponseReceived(const QString& message);
  void onErrorOccurred(const QString& error);
};
