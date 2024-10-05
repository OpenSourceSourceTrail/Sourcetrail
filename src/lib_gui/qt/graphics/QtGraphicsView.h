#pragma once
#include <memory>

#include <QGraphicsView>

#include "MessageListener.h"
#include "type/graph/MessageSaveAsImage.h"
#include "types.h"


class GraphFocusHandler;
class QPushButton;
class QTimer;
class QtGraphEdge;
class QtGraphNode;
class QtSelfRefreshIconButton;

class QtGraphicsView
    : public QGraphicsView
    , public MessageListener<MessageSaveAsImage> {
  Q_OBJECT

public:
  QtGraphicsView(GraphFocusHandler* focusHandler, QWidget* parent);

  [[nodiscard]] float getZoomFactor() const;

  void setAppZoomFactor(float appZoomFactor);

  void setSceneRect(const QRectF& rect);

  [[nodiscard]] QtGraphNode* getNodeAtCursorPosition() const;

  [[nodiscard]] QtGraphEdge* getEdgeAtCursorPosition() const;

  void ensureVisibleAnimated(const QRectF& rect, int xmargin = 50, int ymargin = 50);

  void updateZoom(float delta);

  [[nodiscard]] Id getSchedulerId() const override {
    return m_tabId;
  }

protected:
  /**
   * @brief Qt resize event
   *
   * @param pEvent recive event emmited by parent Qt
   */
  void resizeEvent(QResizeEvent* pEvent) override;

  /**
   * @name mouse events
   * @{ */
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  /**  @} */

  /**
   * @name keyboard events
   * @{ */
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  /**  @} */

  void focusInEvent(QFocusEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;

signals:
  void emptySpaceClicked();
  void resized();

  void focusIn();
  void focusOut();

private slots:
  void updateTimer();
  void stopTimer();

  void openInTab();

  QImage toQImage();
  void exportGraph();
  void copyGraph();
  void copyNodeName();

  void collapseNode();
  void expandNode();

  void showInIDE();
  void showDefinition();
  void hideNode();
  void hideEdge();
  void bookmarkNode();

  void zoomInPressed();
  void zoomOutPressed();

  void hideZoomLabel();

  /**
   * @brief Draw the graphs legend
   */
  void legendClicked() const;

private:
  [[nodiscard]] bool moves() const;

  void setZoomFactor(float zoomFactor);
  void updateTransform();

  void handleMessage(MessageSaveAsImage* pMessage) override;

  GraphFocusHandler* m_focusHandler;

  QPoint m_last;

  float m_zoomFactor;
  float m_appZoomFactor = 1.0F;

  bool m_up = false;
  bool m_down = false;
  bool m_left = false;
  bool m_right = false;

  bool m_shift = false;
  bool m_ctrl = false;

  std::wstring m_clipboardNodeName;
  Id m_openInTabNodeId;
  Id m_hideNodeId;
  Id m_hideEdgeId;
  Id m_bookmarkNodeId;
  Id m_collapseNodeId;
  Id m_expandNodeId;

  std::shared_ptr<QTimer> m_timer;
  std::shared_ptr<QTimer> m_timerStopper;
  std::shared_ptr<QTimer> m_zoomLabelTimer;

  QAction* m_openInTabAction;

  QAction* m_copyNodeNameAction;
  QAction* m_collapseAction;
  QAction* m_expandAction;
  QAction* m_showInIDEAction;
  QAction* m_showDefinitionAction;
  QAction* m_hideNodeAction;
  QAction* m_hideEdgeAction;
  QAction* m_bookmarkNodeAction;

  QAction* m_exportGraphAction;
  QAction* m_copyGraphAction;

  QWidget* m_focusIndicator;

  QPushButton* m_zoomState;
  QtSelfRefreshIconButton* m_zoomInButton;
  QtSelfRefreshIconButton* m_zoomOutButton;

  QtSelfRefreshIconButton* m_pLegendButton;

  float m_zoomInButtonSpeed = 20.0F;
  float m_zoomOutButtonSpeed = -20.0F;

  QImage m_imageCached;
  Id m_tabId;
};
