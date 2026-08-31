#include <csignal>
#include <cstdlib>
#include <tuple>

#include <fmt/core.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

#include "app/Application.h"
#include "app/IndexerPluginRegistry.h"
#include "app/paths/PlatformUserPaths.h"
#include "app/paths/ResourcePaths.h"
#include "AppShell.h"
#include "CommandLineParser.h"
#include "component/view/GraphViewStyle.h"
#include "factory/impls/Factory.hpp"
#include "FilePath.h"
#include "Fonts.h"
#include "logging.h"
#include "network/QtNetworkFactory.h"
#include "productVersion.h"
#include "project/ICxxToolchain.h"
#include "project/SourceGroupFactory.h"
#include "project/SourceGroupFactoryModuleCustom.h"
#include "project/SourceGroupFactoryModuleCxx.h"
#include "project/SourceGroupFactoryModuleJava.h"
#include "QmlGraphViewStyleImpl.h"
#include "ScopedFunctor.h"
#include "settings/ApplicationSettingsPrefiller.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"
#include "type/indexing/MessageIndexingInterrupted.h"
#include "type/MessageLoadProject.h"
#include "type/MessageStatus.h"
#include "utilityApp.h"
#include "Version.h"

namespace {

void signalHandler(int signum) {
  fmt::println("Interrupt signal received. {}", signum);
  MessageIndexingInterrupted{}.dispatch();
}

/**
 * Registers the source-group kinds and discovers the indexer plugins.
 *
 * Same set the engine daemon registers, with one difference: the toolchain is the in-process one.
 * The daemon has to answer compilation-database questions through a remote indexer because it links
 * no language package; this binary is the host, so it uses whatever the plugin registry found.
 */
void addSourceGroupModules() {
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCustom>());
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleJava>());
  SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCxx>());

  IndexerPluginRegistry::getInstance()->discover();
}

}    // namespace

int main(int argc, char* argv[]) {
  // Logging stays off until the settings say otherwise.
  if(auto* logger = spdlog::default_logger_raw(); logger != nullptr) {
    for(auto& sink : logger->sinks()) {
      sink->set_level(spdlog::level::level_enum::off);
    }
  }

  const Version version{VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH};

  QGuiApplication::setApplicationName(QStringLiteral("Sourcetrail"));
  QGuiApplication::setApplicationVersion(QString::fromStdString(version.toString()));
  QGuiApplication::setOrganizationName(QStringLiteral("Coati Software"));

  commandline::CommandLineParser commandLineParser(version.toString());
  std::vector<std::string> args;
  if(argc > 1) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    args = std::vector<std::string>(argv + 1, argv + argc);
  }
  commandLineParser.preparse(std::move(args));
  if(commandLineParser.exitApplication()) {
    return EXIT_SUCCESS;
  }

  const QGuiApplication app(argc, argv);
  QQuickStyle::setStyle(QStringLiteral("Basic"));

  IApplicationSettings::setInstance(std::make_shared<ApplicationSettings>());
  platform_paths::setupPaths();

  // Both of these have to happen before anything measures text. Node boxes are sized from measured
  // text, so the metrics have to be installed before any layout runs -- and the metrics are only
  // right once the vendored families the theme names are actually in the font database. The widget
  // GUI got the style impl from its ViewFactory; there is no view factory any more.
  qml::loadApplicationFonts();
  GraphViewStyle::setImpl(std::make_shared<QmlGraphViewStyleImpl>());

  AppShell shell;
  QtNetworkFactory networkFactory;

  // lib::Factory is the in-process factory: the Project it builds owns a PersistentStorage, so this
  // process is the engine. No supervisor, no channel, no HttpStorageAccess.
  Application::createInstance(version, std::make_shared<lib::Factory>(), &shell, &networkFactory);
  [[maybe_unused]] const ScopedFunctor destroyApplication([]() { Application::destroyInstance(); });

  ApplicationSettingsPrefiller::prefillPaths(IApplicationSettings::getInstanceRaw());
  addSourceGroupModules();

  std::ignore = std::signal(SIGINT, signalHandler);
  std::ignore = std::signal(SIGTERM, signalHandler);

  QQmlApplicationEngine engine;
  engine.loadFromModule("Sourcetrail", "Main");
  if(engine.rootObjects().isEmpty()) {
    fmt::println(stderr, "Failed to load the QML scene.");
    return EXIT_FAILURE;
  }

  MessageStatus{utility::decodeFromUtf8(
                    fmt::format("Starting Sourcetrail {}bit, version {}", utility::getAppArchTypeString(), version.toString()))}
      .dispatch();

  commandLineParser.parse();
  if(commandLineParser.hasError()) {
    Application::getInstance()->handleDialog(commandLineParser.getError());
  } else if(!commandLineParser.getProjectFilePath().empty()) {
    MessageLoadProject{commandLineParser.getProjectFilePath(), false, commandLineParser.getRefreshMode()}.dispatch();
  }

  return QGuiApplication::exec();
}
