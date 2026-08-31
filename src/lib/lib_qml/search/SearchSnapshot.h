#pragma once
#include <QList>
#include <QString>

#include "GlobalId.hpp"

namespace search {

/** One row in the autocompletion list or the search-chip row. */
struct MatchItem final {
  QString name;        ///< The matched text itself.
  QString subtext;     ///< Where it lives -- the enclosing namespace or file.
  QString typeName;    ///< "class", "function", ... as the badge shows it.
  int nodeType = 0;
  int searchType = 0;
  QList<int> indices;    ///< Character positions that matched, for highlighting.
  QList<qulonglong> tokenIds;
  bool hasChildren = false;
};

}    // namespace search
