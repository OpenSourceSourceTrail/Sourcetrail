#pragma once
#include <QObject>
#include <QString>

#include "view.h"
#include "ViewLayout.h"

class ChatView
    : public QObject
    , public View {
  Q_OBJECT
public:
  explicit ChatView(ViewLayout* viewLayout) noexcept;
  Q_DISABLE_COPY_MOVE(ChatView)
  ~ChatView() override;

  [[nodiscard]] std::string getName() const override;

  void createWidgetWrapper() override;

  void refreshView() override;

signals:
  void messageSubmitted(const QString& text);
  void clearRequested();

public slots:
  virtual void setInputEnabled(bool enabled) = 0;

  virtual void clearInput() = 0;

  virtual void focusInput() = 0;
};
