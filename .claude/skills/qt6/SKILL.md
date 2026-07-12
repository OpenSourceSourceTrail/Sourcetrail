---
name: qt6
description: Sourcetrail Qt 6 GUI layer — lib_gui, Component/MVC pattern (Controller + View), ComponentManager, Tab. Use when working on GUI code, views, windows, or components.
---

# Qt 6 GUI Layer

Qt 6.8.2. All Qt code lives in `src/lib/lib_gui/` (QtApplication, Qt views/windows). The core (`src/lib/lib/`) is Qt-free — never leak Qt types into it.

## Component / MVC (`src/lib/lib/component/`)

- `Component` pairs a `Controller` with a `View`.
- `ComponentManager` owns and routes messages to all active components.
- `Tab` holds per-tab component state.
- Controllers react to messages (see the `messaging` skill); views call back through controller interfaces.
- Concrete Qt view implementations live in `src/lib/lib_gui/`; abstract view interfaces and controllers in `src/lib/lib/component/`.
- View creation goes through abstract factories: `IViewFactory` (`src/lib/lib/factory/`).

## Reading data

GUI components query storage through `StorageAccessProxy` (`src/lib/lib/data/storage/`) — a read-access wrapper around `PersistentStorage` — never through the full storage API directly.
