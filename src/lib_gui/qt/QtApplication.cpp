#include "QtApplication.h"

#include <QFileOpenEvent>

#include "FilePath.h"
#include "type/MessageLoadProject.h"
#include "type/MessageWindowFocus.h"

QtApplication::QtApplication(int& argc, char** argv) : QApplication(argc, argv) {
  connect(this, &QGuiApplication::applicationStateChanged, [](auto state) {
    MessageWindowFocus(state == Qt::ApplicationActive).dispatch();
  });
  Q_INIT_RESOURCE(resources);
}

bool QtApplication::event(QEvent* pEvent) {
  if(pEvent->type() == QEvent::FileOpen) {
    auto* pFileEvent = dynamic_cast<QFileOpenEvent*>(pEvent);
    if(pFileEvent != nullptr) {
      FilePath path(pFileEvent->file().toStdWString());

      if(path.exists() && path.extension() == L".srctrlprj") {
        MessageLoadProject(path).dispatch();
        return true;
      }
    }
  }

  return QApplication::event(pEvent);
}
