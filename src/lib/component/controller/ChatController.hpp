#pragma once
#include <QObject>
#include <QString>

#include <qobject.h>
#include <qtmetamacros.h>

#include "ChatView.hpp"
#include "Controller.h"

class ChatController
    : public QObject
    , public Controller {
  Q_OBJECT
public:
  explicit ChatController() noexcept;

  ~ChatController() noexcept override;

  void clear() override {}

  void sendMessage(const QString& message);

signals:
  void messageToAdd(const QString& message, Role role);
  void inputStateChanged(bool enabled);
  void clearInputRequested();

public slots:
  void onResponseReceived(const QString& message);
};
