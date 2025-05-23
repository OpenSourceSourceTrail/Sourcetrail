#include "QtGraphNodeAccess.h"

#include <QBrush>
#include <QCursor>
#include <QFontMetrics>
#include <QPen>

#include "GraphViewStyle.h"
#include "QtDeviceScaledPixmap.h"
#include "ResourcePaths.h"
#include "TokenComponentAccess.h"
#include "utilityQt.h"

QtGraphNodeAccess::QtGraphNodeAccess(AccessKind accessKind)
    : m_accessKind(accessKind), m_accessIcon(nullptr), m_accessIconSize(16) {
  std::wstring accessString = TokenComponentAccess::getAccessString(m_accessKind);
  this->setName(accessString);
  this->setCursor(Qt::ArrowCursor);
  m_text->hide();

  std::wstring iconFileName;
  switch(m_accessKind) {
  case ACCESS_PUBLIC:
    iconFileName = L"public";
    break;
  case ACCESS_PROTECTED:
    iconFileName = L"protected";
    break;
  case ACCESS_PRIVATE:
    iconFileName = L"private";
    break;
  case ACCESS_DEFAULT:
    iconFileName = L"default";
    break;
  case ACCESS_TEMPLATE_PARAMETER:
  case ACCESS_TYPE_PARAMETER:
    iconFileName = L"template";
    break;
  default:
    break;
  }

  if(!iconFileName.empty()) {
    QtDeviceScaledPixmap pixmap(QString::fromStdWString(
        ResourcePaths::getGuiDirectoryPath().concatenate(L"graph_view/images/" + iconFileName + L".png").wstr()));
    pixmap.scaleToHeight(m_accessIconSize);

    m_accessIcon = new QGraphicsPixmapItem(pixmap.pixmap(), this);    // NOLINT(cppcoreguidelines-owning-memory)
    m_accessIcon->setTransformationMode(Qt::SmoothTransformation);
    m_accessIcon->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
  }
}

QtGraphNodeAccess::~QtGraphNodeAccess() = default;

AccessKind QtGraphNodeAccess::getAccessKind() const {
  return m_accessKind;
}

bool QtGraphNodeAccess::isAccessNode() const {
  return true;
}

void QtGraphNodeAccess::addSubNode(QtGraphNode* node) {
  QtGraphNode::addSubNode(node);
  m_text->show();
}

void QtGraphNodeAccess::updateStyle() {
  GraphViewStyle::NodeStyle style = GraphViewStyle::getStyleOfAccessNode();
  setStyle(style);

  QFont font = m_text->font();
  font.setCapitalization(QFont::AllUppercase);
  m_text->setFont(font);

  if(m_accessIcon != nullptr) {
    m_text->setPos(
        static_cast<qreal>(style.textOffset.x() + static_cast<float>(m_accessIconSize) + 3.0f),
        static_cast<qreal>(style.textOffset.y() + static_cast<float>(m_accessIconSize) - static_cast<float>(style.fontSize)));
    m_accessIcon->setPos(style.textOffset.x(), style.textOffset.y());

    m_accessIcon->setPixmap(utility::colorizePixmap(m_accessIcon->pixmap(), style.color.icon.c_str()));
  } else {
    m_text->setPos(static_cast<qreal>(style.textOffset.x()),
                   static_cast<qreal>(style.textOffset.y() + static_cast<float>(m_accessIconSize) + 2.0f -
                                      static_cast<float>(style.fontSize)));
  }
}

void QtGraphNodeAccess::hideLabel() {
  m_text->hide();
}
