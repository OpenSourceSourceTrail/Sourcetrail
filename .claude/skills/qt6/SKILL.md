---
name: qt6
description: Sourcetrail Qt 6 / QML GUI layer — lib_qml, AppShell, view-models, the bus-thread hop, and the toolkit-free controllers it drives. Use when working on GUI code, QML, view-models, or dialogs.
---

# Qt 6 / QML GUI Layer

Qt 6.10, **Qt Quick — no Qt Widgets**. All GUI code lives in `src/lib/lib_qml/`; the executable is
`src/app/qml_gui/`. The core (`src/lib/lib/`) is Qt-free — never leak Qt types into it. A ctest,
`guard.gui.linksNoWidgets`, fails the build if `QWidget` symbols reappear in the binary.

The GUI links `Sourcetrail_lib_engine`: the engine is a library here, not a separate process, and
this process owns the SQLite index. Only indexing is out of process.

## The seam to Application

`Application` knows nothing about QML. It reaches the interface through `lib::IAppShell`
(`src/lib/lib/app/IAppShell.hpp`), thirteen methods with no toolkit in them; `AppShell` in `lib_qml`
is the implementation and also the QML singleton (`QML_ELEMENT` + `QML_SINGLETON`) that the scene
binds to. Passing `nullptr` instead makes the Application headless — that is how the CLI and the
daemon run.

## Threading — the rule that matters most

The message queue runs on **its own thread** (`MessageQueue::startMessageLoopThreaded`) and calls
into view-models from it. Two consequences:

1. **Never touch QML from a message handler.** Hop with `qml::postToGui(this, lambda)`
   (`GuiThread.h`), which posts a queued invocation and returns immediately. It deliberately does
   *not* wait: the widget GUI's `QtThreadedFunctor` held a `QSemaphore(1)`, so the bus thread
   blocked on the GUI thread for every single view update.
2. **Never query storage from the GUI thread.** Read on the bus thread, then move an owned value
   snapshot across the hop. Passing a reference into bus-thread data is a use-after-free waiting to
   happen — by the time the lambda runs, that thread has moved on.

`Q_INVOKABLE` methods called from QML must return immediately too: dispatch a message, or use
`ControllerProxy::executeAsTask`, and let the result arrive as a property or model change.

## Component / MVC (`src/lib/lib/component/`)

- `Component` pairs a `Controller` with a `View`.
- Controllers hold the logic and are toolkit-free. `GraphController` plus the `BucketLayouter` /
  `ListLayouter` / `TrailLayouter` helpers compute the entire graph layout in absolute coordinates;
  `CodeController` and `SnippetMerger` do the same for code snippets. **Reuse them; do not
  reimplement layout in QML.**
- The `*View` classes are the abstract interfaces a controller pushes results at. A view-model owns a
  private implementation of the one it needs and turns what it receives into a
  `QAbstractItemModel` for the scene. `HeadlessGraphView` in `src/app/engine/GraphLayoutService.cpp`
  is the worked example of driving `GraphController` without a widget.

## QML

The scene lives in `src/lib/lib_qml/qml/` and is compiled into the binary by `qt_add_qml_module`
(URI `Sourcetrail`). Register types with `QML_ELEMENT`, never `qmlRegisterType` — that is what lets
the QML type compiler resolve bindings ahead of time. Colours and metrics come from the `Theme`
singleton; a singleton `.qml` file needs `set_source_files_properties(... QT_QML_SINGLETON_TYPE TRUE)`
or every binding against it silently evaluates to `undefined` at runtime.

Because the module is a static library, the executable must link the generated
`Sourcetrail_lib_qmlplugin` explicitly, or the engine reports `No module named "Sourcetrail" found`.

## Reading data

View-models query storage through `StorageCache` (`Application::getStorageCache()`), the cached
`StorageAccess` proxy — never through `PersistentStorage` directly.
