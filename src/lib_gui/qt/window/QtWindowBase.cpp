#include "QtWindowBase.h"
// Qt5
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QVBoxLayout>
// internal
#include "IApplicationSettings.hpp"
#include "ResourcePaths.h"

QtWindowBase::QtWindowBase(bool isSubWindow, QWidget* parent)
    : QtWindowStackElement(parent)
    , m_isSubWindow(isSubWindow)
    , m_window(nullptr)
    , m_content(nullptr)
    , m_mousePressedInWindow(false)
    , m_sizeGrip(nullptr) {
  if(isSubWindow) {
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
  } else {
    setWindowFlags(Qt::Window);
  }

  m_window = new QWidget(this);    // NOLINT(cppcoreguidelines-owning-memory)
  m_window->setObjectName(QStringLiteral("window"));

  auto* layout = new QVBoxLayout(m_window);    // NOLINT(cppcoreguidelines-owning-memory)
  layout->setSpacing(0);

  m_content = new QWidget;    // NOLINT(cppcoreguidelines-owning-memory)
  layout->addWidget(m_content);

  if(isSubWindow) {
    std::string frameStyle =
        "#window {"
        "	border: 1px solid lightgray;"
        "	border-radius: 15px;"
        "	background: white;"
        "}";
    m_window->setStyleSheet(frameStyle.c_str());

    auto* effect = new QGraphicsDropShadowEffect;    // NOLINT(cppcoreguidelines-owning-memory)
    effect->setBlurRadius(15);
    effect->setXOffset(0);
    effect->setYOffset(5);
    effect->setColor(Qt::darkGray);
    m_window->setGraphicsEffect(effect);

    auto* gripLayout = new QHBoxLayout();    // NOLINT(cppcoreguidelines-owning-memory)
    m_sizeGrip = new QSizeGrip(m_window);    // NOLINT(cppcoreguidelines-owning-memory)
    setSizeGripStyle(true);
    gripLayout->addWidget(new QWidget);    // NOLINT(cppcoreguidelines-owning-memory)
    gripLayout->addWidget(m_sizeGrip);
    layout->addLayout(gripLayout);
  }

  resize(sizeHint());

  moveToCenter();

  this->raise();
}

QSize QtWindowBase::sizeHint() const {
  return {IApplicationSettings::getInstanceRaw()->getWindowBaseWidth(),
          IApplicationSettings::getInstanceRaw()->getWindowBaseHeight()};
}

void QtWindowBase::setSizeGripStyle(bool isBlack) {
  if(m_sizeGrip == nullptr) {
    return;
  }

  const std::wstring path = isBlack ? L"size_grip_black.png" : L"size_grip_white.png";

  m_sizeGrip->setStyleSheet(
      QString::fromStdWString(L"QSizeGrip {"
                              "	max-height: 16px;"
                              "	max-width: 16px;"
                              "	border-image: url(" +
                              ResourcePaths::getGuiDirectoryPath().wstr() + L"window/" + path +
                              L");"
                              "}"));
}

bool QtWindowBase::isSubWindow() const {
  return m_isSubWindow;
}

void QtWindowBase::moveToCenter() {
  if(parentWidget() != nullptr) {
    if(m_isSubWindow) {
      move(parentWidget()->width() / 2 - sizeHint().width() / 2, parentWidget()->height() / 2 - sizeHint().height() / 2);
    } else {
      move(parentWidget()->pos().x() + parentWidget()->width() / 2 - sizeHint().width() / 2,
           parentWidget()->pos().y() + parentWidget()->height() / 2 - sizeHint().height() / 2);
    }
  }
}

void QtWindowBase::showWindow() {
  show();
}

void QtWindowBase::hideWindow() {
  hide();
}

void QtWindowBase::mouseMoveEvent(QMouseEvent* event) {
  if(event->buttons() & Qt::LeftButton && m_mousePressedInWindow) {
    if(m_isSubWindow) {
      QPoint pos = event->globalPos() - m_dragPosition;
      QRect parentRect = parentWidget()->rect();

      if(pos.x() < parentRect.left()) {
        pos.setX(parentRect.left());
      }
      pos.setX(qBound(parentRect.left(), parentRect.right() - width(), pos.x()));
      pos.setY(qBound(parentRect.top(), parentRect.bottom() - height(), pos.y()));

      move(pos);
      event->accept();
    } else {
      move(event->globalPos() - m_dragPosition);
      event->accept();
    }
  }
}

void QtWindowBase::mousePressEvent(QMouseEvent* event) {
  if(event->button() == Qt::LeftButton) {
    m_dragPosition = event->globalPos() - frameGeometry().topLeft();
    event->accept();
    m_mousePressedInWindow = true;
  }
}

void QtWindowBase::mouseReleaseEvent(QMouseEvent* event) {
  if(event->button() == Qt::LeftButton) {
    m_dragPosition = event->globalPos() - frameGeometry().topLeft();
    event->accept();
    m_mousePressedInWindow = false;
  }
}
