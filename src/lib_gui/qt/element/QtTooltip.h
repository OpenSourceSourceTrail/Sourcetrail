#ifndef QT_TOOLTIP_H
#define QT_TOOLTIP_H

#include <QFrame>

struct TooltipInfo;

class QtTooltip : public QFrame {
  Q_OBJECT

public:
  QtTooltip(QWidget* parent = nullptr);
  virtual ~QtTooltip();

  void setTooltipInfo(const TooltipInfo& info);

  void setParentView(QWidget* parentView);

  bool isHovered() const;

public slots:
  virtual void show();
  virtual void hide(bool force = false);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
  void enterEvent(QEnterEvent* event) override;
#else
  void enterEvent(QEvent* event) override;
#endif
  void leaveEvent(QEvent* event) override;

private:
  void addTitle(const QString& title, int count, const QString& countText);
  void addWidget(QWidget* widget);

  void clearLayout(QLayout* layout);

  QWidget* m_parentView = nullptr;
  QPoint m_offset;

  bool m_isHovered = false;
};

#endif    // QT_TOOLTIP_H
