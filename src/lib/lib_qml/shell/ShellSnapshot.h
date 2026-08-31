#pragma once
#include <QList>
#include <QString>

#include "GlobalId.hpp"

namespace shell {

/** One entry in the navigation history, as the toolbar's back/forward menu shows it. */
struct HistoryItem final {
  QString name;
  QString typeName;
  int nodeType = 0;
  bool isCurrent = false;
};

/** Everything the toolbar's navigation controls need, in one value the bus thread can hand over. */
struct HistorySnapshot final {
  QList<HistoryItem> items;
  int currentIndex = -1;
  bool canUndo = false;
  bool canRedo = false;
};

/** Everything the status bar shows, likewise. */
struct StatusSnapshot final {
  QString message;
  bool isError = false;
  bool showLoader = false;
  QString ideStatus;
  int errorTotal = 0;
  int errorFatal = 0;
  int indexingPercent = -1;    ///< -1 when no index run is in progress.
};

}    // namespace shell
