#ifndef QT_GRAPH_VIEW_STYLE_IMPL_H
#define QT_GRAPH_VIEW_STYLE_IMPL_H

#include "GraphViewStyleImpl.h"

class QFont;

class QtGraphViewStyleImpl : public GraphViewStyleImpl {
public:
  ~QtGraphViewStyleImpl() override;

  float getCharWidth(const std::string& fontName, size_t fontSize) override;
  float getCharHeight(const std::string& fontName, size_t fontSize) override;
  float getGraphViewZoomDifferenceForPlatform() override;
};

#endif    // QT_GRAPH_VIEW_STYLE_IMPL_H
