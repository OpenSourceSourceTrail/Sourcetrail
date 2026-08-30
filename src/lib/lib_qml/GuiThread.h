#pragma once
#include <utility>

#include <QMetaObject>
#include <QObject>

namespace qml {

/**
 * Hands work to the GUI thread without waiting for it.
 *
 * The message bus runs on its own thread (MessageQueue::startMessageLoopThreaded), so every
 * view-model is called from a thread that must not touch QML. The widget GUI marshalled through
 * QtThreadedFunctor, whose QSemaphore(1) made the *bus thread block* on the GUI thread for every
 * single view update -- the reason indexing progress used to make the whole application crawl.
 * This does the same hop with a plain queued connection and no back-pressure.
 *
 * Callers must therefore pass owned copies, never references into caller-owned data: by the time
 * the lambda runs, the bus thread has moved on.
 */
template <typename Callable>
void postToGui(QObject* context, Callable&& callable) {
  QMetaObject::invokeMethod(context, std::forward<Callable>(callable), Qt::QueuedConnection);
}

}    // namespace qml
