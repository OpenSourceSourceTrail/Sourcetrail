#pragma once
#include <QObject>
#include <QString>

#include "view.h"
#include "ViewLayout.h"


enum class Role : uint8_t { User, Assistant, Error };

class ChatView
    : public QObject
    , public View {
  Q_OBJECT
public:
  explicit ChatView(ViewLayout* viewLayout) noexcept;
  ~ChatView() override;

  [[nodiscard]] std::string getName() const override;

signals:
  void messageSendRequested(const QString& text);

public slots:
  virtual void addMessage(const QString& text, Role role) = 0;

  virtual void setInputEnabled(bool enabled) = 0;

  virtual void clearInput() = 0;

  virtual void clearChat() = 0;

  virtual void displayError(const QString& error) = 0;
};
