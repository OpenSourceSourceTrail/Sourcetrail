#include "TabId.h"

constexpr Id AppId = 1;
constexpr Id BackgroundId = 2;
constexpr Id IgnoreId = 3;
constexpr Id StartingTabId = 10;

Id TabId::s_nextTabId = StartingTabId;
Id TabId::s_currentTabId = 0;

Id TabId::app() {
  return AppId;
}

Id TabId::background() {
  return BackgroundId;
}

Id TabId::ignore() {
  return IgnoreId;
}

Id TabId::nextTab() {
  return s_nextTabId++;
}

Id TabId::currentTab() {
  return s_currentTabId;
}

void TabId::setCurrentTabId(Id currentTabId) {
  s_currentTabId = currentTabId;
}
