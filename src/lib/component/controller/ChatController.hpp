#pragma once
#include <memory>

#include <QObject>
#include <QString>

#include <qobject.h>
#include <qtmetamacros.h>

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

public slots:
  void onResponseReceived(const QString& message);
};
