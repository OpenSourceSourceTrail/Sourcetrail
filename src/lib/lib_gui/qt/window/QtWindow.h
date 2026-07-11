#pragma once

#include "QtWindowBase.h"

class QLabel;
class QPushButton;
class QVBoxLayout;

class QHBoxLayout;

class QtWindow : public QtWindowBase {
  // NOLINTNEXTLINE
  Q_OBJECT

public:
  explicit QtWindow(bool isSubWindow, QWidget* parent = nullptr);

  void setup();

  void setCancelAble(bool cancelAble);
  void setScrollAble(bool scrollAble);

  [[nodiscard]] bool isScrollAble() const;

  void updateTitle(const QString& title);
  [[nodiscard]] std::wstring getTitle() const;
  void updateSubTitle(const QString& subTitle);

  void updateNextButton(const QString& text);
  void updatePreviousButton(const QString& text);
  void updateCloseButton(const QString& text);

  void setNextEnabled(bool enabled);
  void setPreviousEnabled(bool enabled);
  void setCloseEnabled(bool enabled);

  void setNextVisible(bool visible);
  void setPreviousVisible(bool visible);
  void setCloseVisible(bool visible);

  void setNextDefault(bool isDefault);
  void setPreviousDefault(bool isDefault);
  void setCloseDefault(bool isDefault);

signals:
  void finished();
  void canceled();

  void next();
  void previous();

protected:
  void closeEvent(QCloseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

  virtual void populateWindow(QWidget* widget);
  virtual void windowReady();

  virtual void handleNext();
  virtual void handlePrevious();
  virtual void handleClose();

  void setupDone();
  void addLogo();
  QHBoxLayout* createButtons();

  QLabel* m_title = nullptr;
  QLabel* m_subTitle = nullptr;

  QPushButton* m_nextButton = nullptr;
  QPushButton* m_previousButton = nullptr;
  QPushButton* m_closeButton = nullptr;

public slots:
  /**
   * @brief Move to next page
   *
   * @param checked
   */
  void handleNextPress(bool checked);

  /**
   * @brief Move to previous page
   */
  void handlePreviousPress();

  /**
   * @brief Handles the "Close" button press event.
   */
  void handleClosePress();

private:
  bool m_cancelAble = true;
  bool m_scrollAble = false;
  bool m_hasLogo = false;
};
