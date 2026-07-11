#pragma once
// STL
#include <vector>
// Qt5
#include <QWidget>

class QtWindowStackElement : public QWidget {
  // NOLINTNEXTLINE
  Q_OBJECT
public:
  explicit QtWindowStackElement(QWidget* parent = nullptr);

  virtual void showWindow() = 0;
  virtual void hideWindow() = 0;
};


class QtWindowStack : public QObject {
  // NOLINTNEXTLINE
  Q_OBJECT

signals:
  void empty();
  void pop();
  void push();

public:
  explicit QtWindowStack(QObject* parent = nullptr);

  [[nodiscard]] QtWindowStackElement* getTopWindow() const;
  [[nodiscard]] QtWindowStackElement* getBottomWindow() const;

  [[nodiscard]] size_t getWindowCount() const;

  void centerSubWindows();

public slots:
  void pushWindow(QtWindowStackElement* window);
  void popWindow();
  void clearWindows();

private:
  std::vector<QtWindowStackElement*> m_stack;
};
