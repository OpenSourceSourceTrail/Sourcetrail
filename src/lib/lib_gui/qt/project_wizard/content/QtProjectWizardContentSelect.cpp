#include "qt/project_wizard/content/QtProjectWizardContentSelect.h"

#include <regex>

#include <QButtonGroup>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QVariant>

#include "app/paths/ResourcePaths.h"
#include "Capabilities.h"
#include "data/storage/StorageVersion.h"
#include "qt/project_wizard/QtProjectWizardWindow.h"
#include "qt/utility/QtFlowLayout.h"
#include "settings/LanguageType.h"
#include "utility/globalStrings.h"
#include "utilityString.h"

QtProjectWizardContentSelect::QtProjectWizardContentSelect(QtProjectWizardWindow* window) : QtProjectWizardContent(window) {}

void QtProjectWizardContentSelect::populate(QGridLayout* layout, int& /*row*/) {
  struct SourceGroupInfo {
    explicit SourceGroupInfo(SourceGroupType type_, bool recommended_ = false) : type(type_), recommended(recommended_) {}
    const SourceGroupType type;
    const bool recommended;
  };

  // define which kind of source groups are available for each language, gated by what the engine
  // reports it can index -- the plugin directory belongs to the engine, not to this process
  const client::Capabilities& capabilities = client::Capabilities::instance();
  std::map<LanguageType, std::vector<SourceGroupInfo>> sourceGroupInfos;
  if(capabilities.supportsSourceGroupType(SOURCE_GROUP_CXX_CDB)) {
    sourceGroupInfos[LANGUAGE_C].emplace_back(SOURCE_GROUP_CXX_CDB, true);
    sourceGroupInfos[LANGUAGE_CPP].emplace_back(SOURCE_GROUP_CXX_CDB, true);
  }
  if(capabilities.supportsSourceGroupType(SOURCE_GROUP_CXX_VS)) {
    sourceGroupInfos[LANGUAGE_C].emplace_back(SOURCE_GROUP_CXX_VS);
    sourceGroupInfos[LANGUAGE_CPP].emplace_back(SOURCE_GROUP_CXX_VS);
  }
  if(capabilities.supportsSourceGroupType(SOURCE_GROUP_C_EMPTY)) {
    sourceGroupInfos[LANGUAGE_C].emplace_back(SOURCE_GROUP_C_EMPTY);
  }
  if(capabilities.supportsSourceGroupType(SOURCE_GROUP_CPP_EMPTY)) {
    sourceGroupInfos[LANGUAGE_CPP].emplace_back(SOURCE_GROUP_CPP_EMPTY);
  }
  if(capabilities.supportsSourceGroupType(SOURCE_GROUP_JAVA_EMPTY)) {
    sourceGroupInfos[LANGUAGE_JAVA].emplace_back(SOURCE_GROUP_JAVA_EMPTY, true);
  }
  // Custom Command needs no indexer binary, so the engine reports it even on a bare install -- but
  // with no engine there is nothing to run it either, hence the same gate as everything else.
  if(capabilities.supportsSourceGroupType(SOURCE_GROUP_CUSTOM_COMMAND)) {
    sourceGroupInfos[LANGUAGE_CUSTOM].emplace_back(SOURCE_GROUP_CUSTOM_COMMAND);
  }

  // define which icons should be used for which kind of source group
  m_sourceGroupTypeIconName[SOURCE_GROUP_C_EMPTY] = L"empty_icon";
  m_sourceGroupTypeIconName[SOURCE_GROUP_CPP_EMPTY] = L"empty_icon";
  m_sourceGroupTypeIconName[SOURCE_GROUP_CXX_CDB] = L"cdb_icon";
  m_sourceGroupTypeIconName[SOURCE_GROUP_CXX_VS] = L"vs_icon";
  m_sourceGroupTypeIconName[SOURCE_GROUP_JAVA_EMPTY] = L"empty_icon";
  m_sourceGroupTypeIconName[SOURCE_GROUP_CUSTOM_COMMAND] = L"empty_icon";

  // define descriptions for each kind of Source Group
  m_sourceGroupTypeDescriptions[SOURCE_GROUP_C_EMPTY] = "Create a new Source Group by defining which C files will be indexed.";
  m_sourceGroupTypeDescriptions[SOURCE_GROUP_CPP_EMPTY] =
      "Create a new Source Group by defining which C++ files will be indexed.";
  m_sourceGroupTypeDescriptions[SOURCE_GROUP_CXX_CDB] =
      std::string{
          "Create a Source Group from an existing Compilation Database file (compile_commands.json). "
          "It can be exported from CMake<br />(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON) and Make projects "
          "or from the Qt Creator since version 4.8. Have a look at the "
          "<a href=\""} +
      "documentation"_g + "#cc-source-group-from-compilation-database\">documentation</a>.";
  m_sourceGroupTypeDescriptions[SOURCE_GROUP_CXX_VS] =
      std::string{
          "<p>Create a new Source Group from an existing Visual Studio Solution file.</p>"
          "<p><b>Note</b>: Requires a running Visual Studio instance with the "
          "<a href=\""} +
      "documentation"_g + "#visual-studio\">Sourcetrail Visual Studio Extension</a> installed.</p>";
  m_sourceGroupTypeDescriptions[SOURCE_GROUP_JAVA_EMPTY] =
      "Create a new Source Group by defining which Java files will be indexed.";
  m_sourceGroupTypeDescriptions[SOURCE_GROUP_CUSTOM_COMMAND] =
      "Create a new Source Group executing a custom command on each source file. "
      "This Source Group type can be used on <a "
      "href=\"https://github.com/CoatiSoftware/SourcetrailDB\">SourcetrailDB</a> binaries that "
      "add "
      "custom language support to Sourcetrail.<br /><br />Current Database Version: " +
      std::to_string(kStorageVersion);

  auto* vlayout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  vlayout->setContentsMargins(0, 10, 0, 0);
  vlayout->setSpacing(10);

  m_languages = new QButtonGroup;    // NOLINT(cppcoreguidelines-owning-memory)
  for(auto& soureGroupItem : sourceGroupInfos) {
    auto* button = new QPushButton(languageTypeToString(soureGroupItem.first).c_str(), this);
    button->setObjectName(QStringLiteral("menuButton"));
    button->setCheckable(true);
    button->setProperty("language_type", soureGroupItem.first);
    m_languages->addButton(button);
    vlayout->addWidget(button);
  }

  vlayout->addStretch();
  layout->addLayout(vlayout, 0, QtProjectWizardWindow::FRONT_COL, Qt::AlignRight);

  connect(m_languages, &QButtonGroup::buttonClicked, [this](QAbstractButton* button) {
    LanguageType selectedLanguage = LANGUAGE_UNKNOWN;
    bool ok = false;
    int languageTypeInt = button->property("language_type").toInt(&ok);
    if(ok) {
      selectedLanguage = LanguageType(languageTypeInt);
    }

    bool hasRecommended = false;
    for(auto& [languageType, buttonGroup] : m_buttons) {
      buttonGroup->setExclusive(false);
      for(QAbstractButton* abstractButton : buttonGroup->buttons()) {
        abstractButton->setChecked(false);
        abstractButton->setVisible(languageType == selectedLanguage);

        if(languageType == selectedLanguage) {
          hasRecommended = hasRecommended | abstractButton->property("recommended").toBool();
        }
      }
      buttonGroup->setExclusive(true);
    }

    m_window->setNextEnabled(false);
    m_title->setText("Source Group Types - " + m_languages->checkedButton()->text());

    m_description->setText(hasRecommended ? QStringLiteral("<b>* recommended</b>") : QLatin1String(""));
  });

  auto* flayout = new QtFlowLayout(10, 0, 0);    // NOLINT(cppcoreguidelines-owning-memory)

  for(auto& languageIt : sourceGroupInfos) {
    auto* sourceGroupButtons = new QButtonGroup(this);    // NOLINT(cppcoreguidelines-owning-memory)

    for(auto& sourceGroupIt : languageIt.second) {
      std::string name = sourceGroupTypeToProjectSetupString(sourceGroupIt.type);
      if(sourceGroupIt.recommended) {
        name += "*";
      }

      QToolButton* toolButton = createSourceGroupButton(
          utility::insertLineBreaksAtBlankSpaces(name, 15).c_str(),
          QString::fromStdWString(ResourcePaths::getGuiDirectoryPath()
                                      .concatenate(L"icon/" + m_sourceGroupTypeIconName[sourceGroupIt.type] + L".png")
                                      .wstr()));

      if(sourceGroupIt.recommended) {
        toolButton->setStyleSheet(QStringLiteral("font-weight: bold"));
      }

      toolButton->setProperty("source_group_type", int(sourceGroupIt.type));
      toolButton->setProperty("recommended", sourceGroupIt.recommended);
      sourceGroupButtons->addButton(toolButton);
      flayout->addWidget(toolButton);
    }

    m_buttons[languageIt.first] = sourceGroupButtons;
  }

  for(auto& button : m_buttons) {
    connect(button.second, &QButtonGroup::buttonClicked, [this](QAbstractButton* btn) {
      SourceGroupType selectedType = SOURCE_GROUP_UNKNOWN;
      bool ok = false;
      int selectedTypeInt = btn->property("source_group_type").toInt(&ok);
      if(ok) {
        selectedType = SourceGroupType(selectedTypeInt);
      }

      m_description->setText(m_sourceGroupTypeDescriptions[selectedType].c_str());

      m_window->setNextEnabled(true);
    });
  }

  auto* container = new QWidget;              // NOLINT(cppcoreguidelines-owning-memory)
  auto* containerLayout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory)
  containerLayout->setContentsMargins(0, 0, 0, 0);

  QFrame* groupContainer = new QFrame;    // NOLINT(cppcoreguidelines-owning-memory)
  groupContainer->setObjectName(QStringLiteral("sourceGroupContainer"));
  groupContainer->setLayout(flayout);
  containerLayout->addWidget(groupContainer, 0);

  m_description = new QLabel(QStringLiteral(" \n \n"));    // NOLINT(cppcoreguidelines-owning-memory)
  m_description->setWordWrap(true);
  m_description->setOpenExternalLinks(true);
  m_description->setObjectName(QStringLiteral("sourceGroupDescription"));
  m_description->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
  m_description->setMinimumHeight(80);
  containerLayout->addWidget(m_description, 0);

  container->setLayout(containerLayout);
  layout->addWidget(container, 0, QtProjectWizardWindow::BACK_COL);

  m_title = new QLabel(QStringLiteral("Source Group Types"));    // NOLINT(cppcoreguidelines-owning-memory)
  m_title->setObjectName(QStringLiteral("sourceGroupTitle"));

  layout->addWidget(m_title, 0, QtProjectWizardWindow::BACK_COL, Qt::AlignLeft | Qt::AlignTop);

  layout->setRowStretch(0, 0);
  layout->setColumnStretch(QtProjectWizardWindow::FRONT_COL, 0);
  layout->setColumnStretch(QtProjectWizardWindow::BACK_COL, 1);
  layout->setHorizontalSpacing(0);

  // no engine, or an engine with no indexer plugins, means no language buttons at all
  if(const QList<QAbstractButton*> languageButtons = m_languages->buttons(); !languageButtons.isEmpty()) {
    languageButtons.constFirst()->click();
  } else {
    m_description->setText(QStringLiteral(
        "No indexers are available. Sourcetrail cannot create a Source Group until an engine reporting "
        "at least one indexer plugin is connected."));
    m_window->setNextEnabled(false);
  }
}

void QtProjectWizardContentSelect::save() {
  SourceGroupType selectedType = SOURCE_GROUP_UNKNOWN;

  for(const auto& [languageType, buttonGroup] : m_buttons) {
    if(QAbstractButton* button = buttonGroup->checkedButton()) {
      bool ok = false;
      int selectedTypeInt = button->property("source_group_type").toInt(&ok);
      if(ok) {
        selectedType = SourceGroupType(selectedTypeInt);
        break;
      }
    }
  }
  emit selected(selectedType);
}

bool QtProjectWizardContentSelect::check() {
  bool sourceGroupChosen = false;

  for(auto& button : m_buttons) {
    if(button.second->checkedId() != -1) {
      sourceGroupChosen = true;
      break;
    }
  }

  if(!sourceGroupChosen) {
    QMessageBox msgBox(m_window);
    msgBox.setText(QStringLiteral("Please choose a method of creating a new Source Group."));
    msgBox.exec();
    return false;
  }

  return true;
}
