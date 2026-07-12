#pragma once
// internal
#include "QtWindow.h"

class QtProjectWizardContent;

/**
 * @brief
 */
class QtProjectWizardWindow : public QtWindow {
  // NOLINTNEXTLINE(readability-identifier-length)
  Q_OBJECT

public:
  /**
   * @brief Constructor
   *
   * @param pParent pointer to parent
   * @param showSeparator
   */
  explicit QtProjectWizardWindow(QWidget* pParent, bool showSeparator = true);

  [[nodiscard]] QSize sizeHint() const override;

  [[nodiscard]] QtProjectWizardContent* content() const;

  void setContent(QtProjectWizardContent* pContent);

  void setPreferredSize(QSize size);

  void saveContent();

  void loadContent();

  void refreshContent();

  static const int FRONT_COL = 0;
  static const int HELP_COL = 1;
  static const int LINE_COL = 2;
  static const int BACK_COL = 3;

protected:
  /**
   * @name QtWindow implementation
   * @{ */
  void populateWindow(QWidget* widget) override;

  void windowReady() override;

  void handleNext() override;

  void handlePrevious() override;
  /**  @} */

private:
  QtProjectWizardContent* m_content;
  QSize m_preferredSize;
  bool m_showSeparator;
};
