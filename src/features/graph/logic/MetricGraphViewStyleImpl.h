#pragma once

#include <cstddef>
#include <string>

#include "graph/logic/GraphViewStyleImpl.h"

/**
 * Font metrics supplied by the client instead of measured locally.
 *
 * The Qt front end measures its own font through QFontMetrics (QtGraphViewStyleImpl). A front end
 * that only *paints* the layout cannot do that engine-side: the engine has no font stack, and even
 * if it had one it would be measuring a different renderer's font than the one the boxes are drawn
 * in. So the client measures the font it will actually draw with and sends the two numbers the
 * layout needs.
 *
 * GraphViewStyle asks for metrics at several font sizes; only one is measured, and the rest scale
 * linearly from it. That is the same average-advance-width model QtGraphViewStyleImpl uses -- it
 * measures one 37-character string and divides -- so this is no coarser than what the Qt view
 * already lays out with.
 */
class MetricGraphViewStyleImpl : public GraphViewStyleImpl {
public:
  MetricGraphViewStyleImpl(float charWidth, float charHeight, size_t referenceFontSize)
      : mCharWidth(charWidth), mCharHeight(charHeight), mReferenceFontSize(referenceFontSize ? referenceFontSize : 1) {}

  float getCharWidth(const std::string& /*fontName*/, size_t fontSize) override {
    return mCharWidth * scale(fontSize);
  }

  float getCharHeight(const std::string& /*fontName*/, size_t fontSize) override {
    return mCharHeight * scale(fontSize);
  }

  /**
   * 1.0: the Qt value corrects for Qt's own DPI handling on non-Mac platforms, which a client
   * measuring its own rendered text has already accounted for.
   */
  float getGraphViewZoomDifferenceForPlatform() override {
    return 1.0F;
  }

private:
  [[nodiscard]] float scale(size_t fontSize) const {
    return static_cast<float>(fontSize) / static_cast<float>(mReferenceFontSize);
  }

  float mCharWidth;
  float mCharHeight;
  size_t mReferenceFontSize;
};
