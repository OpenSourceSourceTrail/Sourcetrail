# QtStartScreen

## Idea
QtStarScreen is the welcome screen.
- It shows the logo.
- It shows the version.
- It contains a link to the home page.
- It notifies the user if a new version exists.
- It contains a new project button.
- It contains an open project button.
- It contains a list of recent project.

## Use Case

```plantuml
@startuml

User --> (QtStartScreen)

@enduml
```

## Sequence

```plantuml
@startuml

User -> QtStartScreen : Open Project
QtStartScreen -> QtMainWindow : emit signal
QtMainWindow -> QtFileDialog : 
QtFileDialog --> QtMainWindow :
QtMainWindow -> MessageLoadProject : Dispatch

@enduml
```

## Class

```plantuml
@startuml

class QtStartScreen {
+ QtStartScreen()
+ QSize sizeHint() const
+ void setupStartScreen()
--
<<signals>>
+ void openOpenProjectDialog()
+ void openNewProjectDialog()
--
- mRecentModel : RecentItemModel*
}

class QtMainWindow {
+ showStartScreen()
}

QtStartScreen -u-|> QtWindow
QtStartScreen *-l- QtMainWindow
QtStartScreen -r-> ApplicationSettings
QtStartScreen --> ProjectSettings

@enduml
```