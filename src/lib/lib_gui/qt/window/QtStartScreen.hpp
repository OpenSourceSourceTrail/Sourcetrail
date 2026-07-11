#pragma once
#include "QtWindow.h"

namespace qt {

namespace element::model {
struct RecentItemModel;
}

namespace window {
/**
 * @brief Welcome screen
 *
 * It's a pop-up window which show in top of the main application.
 *
 *   ┌─────────┐
 *   │         │
 *┌──┼  Logo   ┼────────────────┐
 *│  │         │                │
 *│  └─────────┘                │
 *│                             │
 *│  ┌version─┐      ┌Recent──┐ │
 *│  │ Github │      │        │ │
 *│  └────────┘      │        │ │
 *│                  │        │ │
 *│ ┌───────┐        │        │ │
 *│  New Pro         │        │ │
 *│  Open Pro        │        │ │
 *│ └───────┘        └────────┘ │
 *└─────────────────────────────┘
 */
class QtStartScreen final : public QtWindow {
  Q_OBJECT

public:
  /**
   * @brief Constructor.
   *
   * @param pParent parent object parent.
   */
  explicit QtStartScreen(QWidget* pParent = nullptr);

  /**
   * @brief Recommended window size.
   *
   * @return Recommended window size.
   */
  [[nodiscard]] QSize sizeHint() const override;

  /**
   * @brief Initialize the startup window elements.
   */
  void setupStartScreen();

protected:
  void hideEvent(QHideEvent* hideEvent) override;
  void closeEvent(QCloseEvent* closeEvent) override;

signals:
  /**
   * @brief A signal emitted when the open button clicked.
   */
  void openOpenProjectDialog();

  /**
   * @brief A signal emitted when the new button clicked.
   */
  void openNewProjectDialog();

private:    // NOLINT(readability-redundant-access-specifiers)
  element::model::RecentItemModel* mRecentModel = nullptr;

  void createVersionAndGithub(QHBoxLayout* layout);

  void createRecentProjects(QHBoxLayout* layout);
};

}    // namespace window
}    // namespace qt