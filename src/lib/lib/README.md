# Sourcetrail_lib

## Description

It contains the bussines logic.

## Components

- app
- component
- data
- factory
- project
- setttings
- utility

## UMLs

### Application

```mermaid
classDiagram
  class Application {
    +static createInstance()
    -bool mHasGui
    -std::shared_ptr<lib::IFactory> mFactory
    -bool mLoadedWindow
    -std::shared_ptr<IProject> mProject
    -std::shared_ptr<StorageCache> mStorageCache
    -std::shared_ptr<MainView> mMainView
    -std::shared_ptr<IDECommunicationController> mIdeCommunicationController
  }

  Application *-- IFactory
  Application *-- IViewFactory
  Application *-- INetworkFactory
  Application *-- ITaskManager
  Application *-- IMessageQueue
  Application *-- StorageCache
```

### Project

```mermaid
classDiagram
    class IProject {
        <<interface>>
        +getProjectSettingsFilePath() FilePath
        +getDescription() string
        +isLoaded() bool
        +isIndexing() bool
        +settingsEqualExceptNameAndLocation(ProjectSettings) bool
        +setStateOutdated()
        +load(DialogView)
        +refresh(DialogView, RefreshMode, bool)
        +getRefreshInfo(RefreshMode) RefreshInfo
        +buildIndex(RefreshInfo, DialogView)
    }

    class Project {
        -ProjectStateType m_state
        -RefreshStageType m_refreshStage
        -ProjectSettings m_settings
        -StorageCache* m_storageCache
        -PersistentStorage m_storage
        -vector~SourceGroup~ m_sourceGroups
        -string m_appUUID
        -bool m_hasGUI
        +Project(ProjectSettings, StorageCache*, string, bool)
        -swapToTempStorage(DialogView)
        -swapToTempStorageFile(FilePath, FilePath, DialogView) bool
        -discardTempStorage()
        -hasCxxSourceGroup() bool
        -createIndexTasks(RefreshInfo, DialogView, PersistentStorage, size_t&) TaskGroupSequence
        -checkIfNothingToRefresh(RefreshInfo, DialogView) bool
        -checkIfFilesToClear(RefreshInfo, DialogView) bool
    }

    class ProjectStateType {
        <<enumeration>>
        NOT_LOADED
        EMPTY
        LOADED
        OUTDATED
        OUTVERSIONED
        NEEDS_MIGRATION
        DB_CORRUPTED
    }

    class RefreshStageType {
        <<enumeration>>
        REFRESHING
        INDEXING
        NONE
    }

    IProject <|-- Project
    Project *-- ProjectStateType
    Project *-- RefreshStageType
```

### Settings

```mermaid
classDiagram
  class Settings

  class ProjectSettings

  Settings <|-- ProjectSettings
```