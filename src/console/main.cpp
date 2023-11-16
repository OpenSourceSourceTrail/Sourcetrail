#include <filesystem>

#include <fmt/color.h>
#include <fmt/printf.h>

#include <QCommandLineParser>
#include <QCoreApplication>

#include "Project.h"
#include "ProjectSettings.h"
#include "StorageCache.h"
#include "utilityUuid.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
  const QCoreApplication application(argc, argv);

  QCoreApplication::setApplicationName("Sourcetrail");
  QCoreApplication::setApplicationVersion("1.0");

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument(
      "compile_commands", QCoreApplication::translate("compile_commands", "A path to compile_commands.json"));

  parser.process(application);

  const auto args = parser.positionalArguments();
  if(args.isEmpty()) {
    fmt::print(fg(fmt::color::red), "ERROR: Missing the following argument compile_commands.");
    return EXIT_FAILURE;
  }

  std::error_code errorCode;
  const fs::path compileCommandsPath = args.at(0).toStdString();
  if(!fs::exists(compileCommandsPath, errorCode) && !fs::is_regular_file(compileCommandsPath, errorCode)) {
    fmt::print(fg(fmt::color::red), "ERROR: The compileCommands path is not exists or not at file.");
    return EXIT_FAILURE;
  }

  StorageCache storageCache;
  auto emptySettings = std::make_shared<ProjectSettings>();
  if(!emptySettings->load(FilePath{compileCommandsPath.string()}, true)) {
    fmt::print(fg(fmt::color::red), "ERROR: Can not load the project settings.");
    return EXIT_FAILURE;
  }

  Project project(emptySettings, &storageCache, utility::getUuidString(), false);
  project.load(nullptr);

  return QCoreApplication::exec();
}