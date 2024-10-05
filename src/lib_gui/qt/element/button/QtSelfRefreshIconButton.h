#pragma once

#include <QPushButton>
#include <QTimer>

#include "FilePath.h"
#include "MessageListener.h"
#include "type/MessageRefreshUI.h"
#include "QtThreadedFunctor.h"

class QtSelfRefreshIconButton
    : public QPushButton
    , public MessageListener<MessageRefreshUI> {
public:
  QtSelfRefreshIconButton(const QString& text, FilePath iconPath, std::string buttonKey, QWidget* parent = nullptr);
  ~QtSelfRefreshIconButton() override;

  Q_DISABLE_COPY_MOVE(QtSelfRefreshIconButton)

  void setText(const QString& text);
  void setIconPath(const FilePath& iconPath);

  void setAutoElide(bool autoElide);

protected:
  void handleMessage(MessageRefreshUI* message) override;

  virtual void refresh();

  void resizeEvent(QResizeEvent* event) override;

private:
  void updateText(int width);

  QtThreadedLambdaFunctor m_onQtThread;

  QString m_text;
  FilePath m_iconPath;
  std::string m_buttonKey;

  bool m_autoElide = false;
  bool m_blockUpdate = false;
  QTimer m_updateTimer;
  QTimer m_blockTimer;
};