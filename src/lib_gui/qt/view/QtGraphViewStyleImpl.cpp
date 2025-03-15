#include "QtGraphViewStyleImpl.h"

#include <QFont>
#include <QFontMetrics>

#include "utilityApp.h"

namespace {
QFont getFontForStyleType(const std::string& fontName, size_t fontSize) {
  QFont font{fontName.c_str()};
  font.setPixelSize(static_cast<int>(fontSize));
  return font;
}
}    // namespace

QtGraphViewStyleImpl::~QtGraphViewStyleImpl() = default;

float QtGraphViewStyleImpl::getCharWidth(const std::string& fontName, size_t fontSize) {
  return QFontMetrics{getFontForStyleType(fontName, fontSize)}
             .boundingRect(QStringLiteral("QtGraphNode::QtGraphNode::QtGraphNode"))
             .width() /
      37.0f;
}

float QtGraphViewStyleImpl::getCharHeight(const std::string& fontName, size_t fontSize) {
  return static_cast<float>(QFontMetrics(getFontForStyleType(fontName, fontSize)).height());
}

float QtGraphViewStyleImpl::getGraphViewZoomDifferenceForPlatform() {
  if constexpr(utility::getOsType() == OsType::Mac) {
    return 1;
  }

  return 1.25F;
}
