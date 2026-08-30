#include "QmlGraphViewStyleImpl.h"

#include <QFont>
#include <QFontMetricsF>

#include "utilityApp.h"

namespace {

// The same 37-character yardstick the widget GUI measured with; changing it would resize every
// node box in every existing layout.
const auto Yardstick = QStringLiteral("QtGraphNode::QtGraphNode::QtGraphNode");
constexpr float YardstickLength = 37.0F;

QFont fontFor(const std::string& fontName, size_t fontSize) {
  QFont font{QString::fromStdString(fontName)};
  font.setPixelSize(static_cast<int>(fontSize));
  return font;
}

}    // namespace

QmlGraphViewStyleImpl::~QmlGraphViewStyleImpl() = default;

float QmlGraphViewStyleImpl::getCharWidth(const std::string& fontName, size_t fontSize) {
  const auto width = QFontMetricsF(fontFor(fontName, fontSize)).boundingRect(Yardstick).width();
  return static_cast<float>(width) / YardstickLength;
}

float QmlGraphViewStyleImpl::getCharHeight(const std::string& fontName, size_t fontSize) {
  return static_cast<float>(QFontMetricsF(fontFor(fontName, fontSize)).height());
}

float QmlGraphViewStyleImpl::getGraphViewZoomDifferenceForPlatform() {
  if constexpr(utility::getOsType() == OsType::Mac) {
    return 1.0F;
  }

  return 1.25F;
}
