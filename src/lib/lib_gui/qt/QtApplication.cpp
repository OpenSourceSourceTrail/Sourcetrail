#include "QtApplication.h"

#include <QFileOpenEvent>

#include "FilePath.h"
#include "type/MessageLoadProject.h"
#include "type/MessageWindowFocus.h"

QtApplication::QtApplication(int& argc, char** argv) : QApplication(argc, argv) {
  std::ignore = connect(this, &QGuiApplication::applicationStateChanged, this, [](auto state) {
    MessageWindowFocus(state == Qt::ApplicationActive).dispatch();
  });
  Q_INIT_RESOURCE(resources);
}

bool QtApplication::event(QEvent* event) {
  if(event->type() == QEvent::FileOpen) {
    auto* fileEvent = dynamic_cast<QFileOpenEvent*>(event);
    if(fileEvent != nullptr) {
      FilePath path(fileEvent->file().toStdWString());

      if(path.exists() && path.extension() == L".srctrlprj") {
        MessageLoadProject(path).dispatch();
        return true;
      }
    }
  }

  return QApplication::event(event);
}
