#include "QtAbout.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "globalStrings.h"
#include "QtDeviceScaledPixmap.h"
#include "ResourcePaths.h"
#include "SqliteIndexStorage.h"
#include "utilityApp.h"
#include "utilityQt.h"
#include "Version.h"

QtAbout::QtAbout(QWidget* parent) : QtWindow{false, parent} {}

QSize QtAbout::sizeHint() const {
  return {450, 480};
}

void QtAbout::setupAbout() {
  setStyleSheet(utility::getStyleSheet(ResourcePaths::getGuiDirectoryPath().concatenate(L"about/about.css")).c_str());

  auto* windowLayout = new QVBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory): Qt will handles it
  windowLayout->setContentsMargins(10, 10, 10, 0);
  windowLayout->setSpacing(1);
  m_content->setLayout(windowLayout);

  {
    QtDeviceScaledPixmap sourcetrailLogo(
        QString::fromStdWString(ResourcePaths::getGuiDirectoryPath().wstr() + L"about/logo_sourcetrail.png"));
    sourcetrailLogo.scaleToHeight(150);
    auto* sourcetrailLogoLabel = new QLabel{this};    // NOLINT(cppcoreguidelines-owning-memory): Qt will handles it
    sourcetrailLogoLabel->setPixmap(sourcetrailLogo.pixmap());
    sourcetrailLogoLabel->resize(static_cast<int>(sourcetrailLogo.width()), static_cast<int>(sourcetrailLogo.height()));
    windowLayout->addWidget(sourcetrailLogoLabel, 0, Qt::Alignment(Qt::AlignmentFlag::AlignHCenter));
  }

  windowLayout->addSpacing(10);

  {
    auto* versionLabel = new QLabel{
        ("Version " + Version::getApplicationVersion().toString() + " - " +
         std::string(utility::getApplicationArchitectureType() == ApplicationArchitectureType::X86_32 ? "32" : "64") + " bit")
            .c_str(),
        this};
    windowLayout->addWidget(versionLabel, 0, Qt::Alignment(Qt::AlignmentFlag::AlignHCenter));
  }

  {
    auto* dbVersionLabel = new QLabel{"Database Version " + QString::number(SqliteIndexStorage::getStorageVersion()), this};
    windowLayout->addWidget(dbVersionLabel, 0, Qt::Alignment(Qt::AlignmentFlag::AlignHCenter));
  }

  windowLayout->addStretch();

  {
    auto* layoutHorz1 = new QHBoxLayout;    // NOLINT(cppcoreguidelines-owning-memory): Qt will handles it
    windowLayout->addLayout(layoutHorz1);

    layoutHorz1->addStretch();

    auto* developerLabel = new QLabel{
        QString::fromStdString("<b>Authors: <a href=\"%1\" "
                               "style=\"color: "
                               "white;\">%1</a></b>")
            .arg("https://raw.githubusercontent.com/OpenSourceSourceTrail/Sourcetrail/refs/heads/main/AUTHORS.txt")};

    developerLabel->setObjectName(QStringLiteral("small"));
    layoutHorz1->addWidget(developerLabel);

    layoutHorz1->addStretch();
  }

  windowLayout->addStretch();

  {
    auto* acknowledgementsLabel = new QLabel{
        QString::fromStdString("<b>Acknowledgements:</b><br />"
                               "Sourcetrail (aka Coati) 0.1 was created in the context of education at "
                               "<a href=\"http://www.fh-salzburg.ac.at/en/\" style=\"color: white;\">Salzburg "
                               "University "
                               "of Applied Sciences</a>.<br />"
                               "Coati Software KG is member of <a href=\"http://www.startup-salzburg.at/\" "
                               "style=\"color: "
                               "white;\">Startup Salzburg</a>.<br />"
                               "The development of Sourcetrail was funded by <a href=\"http://awsg.at\" "
                               "style=\"color: "
                               "white;\">aws</a>.")};
    acknowledgementsLabel->setObjectName(QStringLiteral("small"));
    acknowledgementsLabel->setWordWrap(true);
    acknowledgementsLabel->setOpenExternalLinks(true);
    windowLayout->addWidget(acknowledgementsLabel);
    windowLayout->addSpacing(10);
  }

  {
    auto* webLabel = new QLabel{QString{"<b>Repository: <a href=\"%1\" "
                                        "style=\"color: "
                                        "white;\">%1</a></b>"}
                                    .arg("github"_g),
                                this};
    webLabel->setObjectName(QStringLiteral("small"));
    webLabel->setOpenExternalLinks(true);
    windowLayout->addWidget(webLabel);
    windowLayout->addSpacing(10);
  }
}
