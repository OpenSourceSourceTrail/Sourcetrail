#include "MouseReleaseFilter.hpp"

#include <QMouseEvent>

#include "IApplicationSettings.hpp"
#include "type/history/MessageHistoryRedo.h"
#include "type/history/MessageHistoryUndo.h"


MouseReleaseFilter::MouseReleaseFilter(QObject* parent)
    : QObject(parent)
    , m_backButton(static_cast<std::size_t>(IApplicationSettings::getInstanceRaw()->getControlsMouseBackButton()))
    , m_forwardButton(static_cast<std::size_t>(IApplicationSettings::getInstanceRaw()->getControlsMouseForwardButton())) {}

bool MouseReleaseFilter::eventFilter(QObject* obj, QEvent* event) {
  if(event->type() == QEvent::MouseButtonRelease) {
    if(const auto* mouseEvent = dynamic_cast<QMouseEvent*>(event); mouseEvent != nullptr) {
      if(mouseEvent->button() == m_backButton) {
        MessageHistoryUndo{}.dispatch();
        return true;
      } else if(mouseEvent->button() == m_forwardButton) {
        MessageHistoryRedo{}.dispatch();
        return true;
      }
    }
  }

  return QObject::eventFilter(obj, event);
}
