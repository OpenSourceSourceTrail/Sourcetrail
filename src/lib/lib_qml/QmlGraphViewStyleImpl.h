#pragma once
#include "component/view/GraphViewStyleImpl.h"

/**
 * Sizes graph node boxes from the font the QML scene actually paints in.
 *
 * QFontMetricsF needs a QGuiApplication, not a QApplication, so this works unchanged without the
 * widget module. The engine daemon keeps using MetricGraphViewStyleImpl, which takes its metrics
 * from the client instead of measuring anything.
 */
class QmlGraphViewStyleImpl final : public GraphViewStyleImpl {
public:
  ~QmlGraphViewStyleImpl() override;

  float getCharWidth(const std::string& fontName, size_t fontSize) override;
  float getCharHeight(const std::string& fontName, size_t fontSize) override;
  float getGraphViewZoomDifferenceForPlatform() override;
};
