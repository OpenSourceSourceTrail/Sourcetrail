#pragma once
// Qt5
#include <QApplication>

/**
 * @brief The QtApplication class manages the GUI application.
 */
class QtApplication : public QApplication {
  // NOLINTNEXTLINE(readability-identifier-length)
  Q_OBJECT
public:
  /**
   * @brief Initializes the window system and constructs an application object with argc command
   * line arguments in argv.
   *
   * @param argc count of commnad argments
   * @param argv a list of argments
   */
  QtApplication(int& argc, char** argv);

  /**
   * @brief This virtual function receives events to an object and should return true if the event e
   * was recognized and processed.
   *
   * @note Reimplements: QApplication::event(QEvent *e)
   * @note responds to FileOpenEvent specific for Mac
   *
   * @param pEvent a base event
   *
   * @return true if the event is handled
   */
  bool event(QEvent* event) override;
};
