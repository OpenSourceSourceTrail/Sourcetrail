#pragma once
#include <memory>

#include <QObject>
#include <QString>

#include "Controller.h"

/*
class LlmInterface : public QObject {
public:
  virtual ~LlmInterface() = default;
  virtual void sendMessage(const QString& message) noexcept = 0;
public slots:
  void onResponseReceived(const QString& response) noexcept;
};
*/

class ChatController : public Controller {
public:
  explicit ChatController() noexcept;

  ~ChatController() noexcept override;

  void clear() override {}
  // public slots:
  void onSendMessage(const QString& message);
  void onResponseReceived(const QString& message);

private:
};
