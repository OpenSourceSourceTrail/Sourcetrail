# Sourcetrail_lib

```plantuml
@startuml
!include https://raw.githubusercontent.com/patrik-csak/one-dark-plantuml-theme/v1.0.1/theme.puml

Settings <|-- ProjectSettings
@enduml
```

## Application
```plantuml
@startuml
!include https://raw.githubusercontent.com/patrik-csak/one-dark-plantuml-theme/v1.0.1/theme.puml

class Application {
+ static createInstance()
- mHasGui : bool
- mFactory : std::shared_ptr<lib::IFactory>
- mLoadedWindow : bool 
- mProject : std::shared_ptr<IProject>
- mStorageCache : std::shared_ptr<StorageCache>
- mMainView : std::shared_ptr<MainView>
- mIdeCommunicationController : std::shared_ptr<IDECommunicationController>
}

Application *-- IFactory
Application *-- IViewFactory
Application *-- INetworkFactory
Application *-- ITaskManager
Application *-- IMessageQueue
Application *-- StorageCache

@enduml
```