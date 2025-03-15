#ifndef QT_TABLE_H
#define QT_TABLE_H

#include <QTableView>

class QtTable : public QTableView {
  Q_OBJECT

public:
  QtTable(QWidget* parent = nullptr);
  ~QtTable() override;

  void updateRows();
  int getFilledRowCount();

  void showFirstRow();
  void showLastRow();

  [[nodiscard]] bool hasSelection() const;

protected slots:
  void columnResized(int column, int oldWidth, int newWidth);

protected:
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

private:
  float m_rowsToFill = 0.0F;

  int m_colIndex = -1;
  int m_lastPos = -1;
};

#endif    // QT_TABLE_H
