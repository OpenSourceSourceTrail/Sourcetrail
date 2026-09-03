#include <fstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "app/Application.h"
#include "app/paths/AppPath.h"
#include "FileSystem.h"
#include "indexing/logic/IndexerCommandCustom.h"
#include "language_packages.h"
#include "MockedApplicationSetting.hpp"
#include "MockedMessageQueue.hpp"
#include "project/logic/SourceGroupCustomCommand.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"
#include "settings/ProjectSettings.h"
#include "settings/source_group/type/SourceGroupSettingsCustomCommand.h"
#include "TextAccess.h"
#include "utility/path_detector/utilityPathDetection.h"
#include "utilityFile.h"
#include "utilityString.h"
#include "Version.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "Blackboard.h"
#  include "data/parser/cxx/CxxParser.h"
#  include "data/parser/ParserClientImpl.h"
#  include "data/storage/IntermediateStorage.h"
#  include "data/storage/StorageProvider.h"
#  include "FileRegister.h"
#  include "indexing/domain/IndexerStateInfo.h"
#  include "indexing/logic/IndexerCommandCxx.h"
#  include "MockedDialogView.hpp"
#  include "project/CxxToolchainLocal.h"
#  include "project/logic/ICxxToolchain.h"
#  include "project/logic/SourceGroupCxxCdb.h"
#  include "project/logic/SourceGroupCxxEmpty.h"
#  include "settings/source_group/type/SourceGroupSettingsCEmpty.h"
#  include "settings/source_group/type/SourceGroupSettingsCppEmpty.h"
#  include "settings/source_group/type/SourceGroupSettingsCxxCdb.h"
#  include "Task.h"
#endif    // BUILD_CXX_LANGUAGE_PACKAGE


#define REQUIRE_MESSAGE(msg, cond)                                                                                               \
  do {                                                                                                                           \
    INFO(msg);                                                                                                                   \
    EXPECT_TRUE(cond);                                                                                                           \
  } while((void)0, 0)

namespace {
const bool updateExpectedOutput = false;

static FilePath getInputDirectoryPath(const std::wstring& projectName) {
  return FilePath(L"data/SourceGroupTestSuite/" + projectName + L"/input").makeAbsolute().makeCanonical();
}

static FilePath getOutputDirectoryPath(const std::wstring& projectName) {
  return FilePath(L"data/SourceGroupTestSuite/" + projectName + L"/expected_output").makeAbsolute().makeCanonical();
}

#if BUILD_CXX_LANGUAGE_PACKAGE
std::wstring indexerCommandCxxToString(std::shared_ptr<const IndexerCommandCxx> indexerCommand, const FilePath& baseDirectory) {
  std::wstring result;
  result += L"SourceFilePath: \"" + indexerCommand->getSourceFilePath().getRelativeTo(baseDirectory).wstr() + L"\"\n";
  for(const FilePath& indexedPath : indexerCommand->getIndexedPaths()) {
    result += L"\tIndexedPath: \"" + indexedPath.getRelativeTo(baseDirectory).wstr() + L"\"\n";
  }
  for(std::wstring compilerFlag : indexerCommand->getCompilerFlags()) {
    FilePath flagAsPath(compilerFlag);
    if(flagAsPath.exists()) {
      compilerFlag = flagAsPath.getRelativeTo(baseDirectory).wstr();
    }
    result += L"\tCompilerFlag: \"" + compilerFlag + L"\"\n";
  }
  for(const FilePathFilter& filter : indexerCommand->getExcludeFilters()) {
    result += L"\tExcludeFilter: \"" + filter.wstr() + L"\"\n";
  }
  return result;
}
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

std::wstring indexerCommandCustomToString(std::shared_ptr<const IndexerCommandCustom> indexerCommand,
                                          const FilePath& baseDirectory) {
  std::wstring result;
  result += L"IndexerCommandCustom\n";
  result += L"\tSourceFilePath: \"" + indexerCommand->getSourceFilePath().getRelativeTo(baseDirectory).wstr() + L"\"\n";
  result += L"\tCustom Command: \"" + indexerCommand->getCommand() + L"\"\n";
  result += L"\tArguments:\n";
  for(const std::wstring& argument : indexerCommand->getArguments()) {
    result += L"\t\t\"" + argument + L"\"\n";
  }
  return result;
}

std::wstring indexerCommandToString(std::shared_ptr<IndexerCommand> indexerCommand, const FilePath& baseDirectory) {
  if(indexerCommand) {
#if BUILD_CXX_LANGUAGE_PACKAGE
    if(std::shared_ptr<const IndexerCommandCxx> indexerCommandCxx = std::dynamic_pointer_cast<const IndexerCommandCxx>(
           indexerCommand)) {
      return indexerCommandCxxToString(indexerCommandCxx, baseDirectory);
    }
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
    if(std::shared_ptr<const IndexerCommandCustom> indexerCommandCustom = std::dynamic_pointer_cast<const IndexerCommandCustom>(
           indexerCommand)) {
      return indexerCommandCustomToString(indexerCommandCustom, baseDirectory);
    }
    return L"Unsupported indexer command type: " +
        utility::decodeFromUtf8(indexerCommandTypeToString(indexerCommand->getIndexerCommandType()));
  }
  return L"No IndexerCommand provided.";
}

std::shared_ptr<TextAccess> generateExpectedOutput(std::wstring projectName, std::shared_ptr<const SourceGroup> sourceGroup) {
  const FilePath projectDataRoot = getInputDirectoryPath(projectName).makeAbsolute();

  RefreshInfo info;
  info.filesToIndex = sourceGroup->getAllSourceFilePaths();
  std::vector<std::shared_ptr<IndexerCommand>> indexerCommands = sourceGroup->getIndexerCommands(info);

  std::sort(
      indexerCommands.begin(), indexerCommands.end(), [](std::shared_ptr<IndexerCommand> a, std::shared_ptr<IndexerCommand> b) {
        return a->getSourceFilePath().wstr() < b->getSourceFilePath().wstr();
      });

  std::wstring outputString;
  for(std::shared_ptr<IndexerCommand> indexerCommand : indexerCommands) {
    outputString += indexerCommandToString(indexerCommand, projectDataRoot);
  }

  return TextAccess::createFromString(utility::encodeToUtf8(outputString));
}

void generateAndCompareExpectedOutput(const std::wstring& projectName, std::shared_ptr<const SourceGroup> sourceGroup) {
  const std::shared_ptr<const TextAccess> output = generateExpectedOutput(projectName, std::move(sourceGroup));
#ifdef WIN32
  const std::wstring expectedOutputFileName = L"output_windows.txt";
#else
  const std::wstring expectedOutputFileName = L"output_unix.txt";
#endif
  const FilePath expectedOutputFilePath = getOutputDirectoryPath(projectName).concatenate(expectedOutputFileName);
  if(!expectedOutputFilePath.exists()) {
    std::ofstream expectedOutputFile;
    expectedOutputFile.open(expectedOutputFilePath.str());
    expectedOutputFile << output->getText();
    expectedOutputFile.close();
  } else {
    const std::shared_ptr<const TextAccess> expectedOutput = TextAccess::createFromFile(expectedOutputFilePath);
    ASSERT_EQ(expectedOutput->getLineCount(), output->getLineCount())
        << "Output does not match the expected line count for project \"" + utility::encodeToUtf8(projectName) +
            "\". Output was: " + output->getText();
    if(expectedOutput->getLineCount() == output->getLineCount()) {
      for(unsigned int i = 1; i <= expectedOutput->getLineCount(); i++) {
        EXPECT_EQ(expectedOutput->getLine(i), output->getLine(i));
      }
    }
  }
}
}    // namespace

struct SourceGroupFix : testing::Test {
  void SetUp() override {
    mMockedApplicationSettings = std::make_shared<testing::StrictMock<MockedApplicationSettings>>();
    IApplicationSettings::setInstance(mMockedApplicationSettings);

    mMockedMessageQueue = std::make_shared<testing::StrictMock<MockedMessageQueue>>();
    IMessageQueue::setInstance(mMockedMessageQueue);

#if BUILD_CXX_LANGUAGE_PACKAGE
    // The compilation database case reads a real compile_commands.json, which needs a real toolchain.
    ICxxToolchain::setInstance(std::make_shared<CxxToolchainLocal>());
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
  }

  void TearDown() override {
#if BUILD_CXX_LANGUAGE_PACKAGE
    ICxxToolchain::setInstance(nullptr);
#endif    // BUILD_CXX_LANGUAGE_PACKAGE
    IApplicationSettings::setInstance(nullptr);
    mMockedApplicationSettings.reset();
    IMessageQueue::setInstance(nullptr);
    mMockedMessageQueue.reset();
  }

  std::shared_ptr<testing::StrictMock<MockedApplicationSettings>> mMockedApplicationSettings;
  std::shared_ptr<testing::StrictMock<MockedMessageQueue>> mMockedMessageQueue;
};

#ifdef DISABLED
TEST(SourceGroupFix, can create application instance) {
  // required to query in SourceGroup for dialog view... this is not a very elegant solution.
  // should be refactored to pass dialog view to SourceGroup on creation.
  Application::createInstance(Version(), nullptr, nullptr);
  EXPECT_TRUE(Application::getInstance().use_count() >= 1);
}
#endif

#if BUILD_CXX_LANGUAGE_PACKAGE
TEST_F(SourceGroupFix, sourceGroupCxxCEmptyGeneratesExpectedOutput) {
  EXPECT_CALL(*mMockedApplicationSettings, getHeaderSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path>{{"test/header/search/path"}}));
  EXPECT_CALL(*mMockedApplicationSettings, getFrameworkSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path>{{"test/framework/search/path"}}));

  const std::wstring projectName = L"cxx_c_empty";

  ProjectSettings projectSettings;
  projectSettings.setProjectFilePath(L"non_existent_project", getInputDirectoryPath(projectName));

  std::shared_ptr<SourceGroupSettingsCEmpty> sourceGroupSettings = std::make_shared<SourceGroupSettingsCEmpty>(
      "fake_id", &projectSettings);
  sourceGroupSettings->setSourcePaths({getInputDirectoryPath(projectName).concatenate(L"src")});
  sourceGroupSettings->setSourceExtensions({L".c"});
  sourceGroupSettings->setExcludeFilterStrings({L"**/excluded/**"});
  sourceGroupSettings->setTargetOptionsEnabled(true);
  sourceGroupSettings->setTargetArch(L"test_arch");
  sourceGroupSettings->setTargetVendor(L"test_vendor");
  sourceGroupSettings->setTargetSys(L"test_sys");
  sourceGroupSettings->setTargetAbi(L"test_abi");
  sourceGroupSettings->setCStandard(L"c11");
  sourceGroupSettings->setHeaderSearchPaths({getInputDirectoryPath(projectName).concatenate(L"header_search/local")});
  sourceGroupSettings->setFrameworkSearchPaths({getInputDirectoryPath(projectName).concatenate(L"framework_search/local")});
  sourceGroupSettings->setCompilerFlags({L"-local-flag"});

  generateAndCompareExpectedOutput(projectName, std::make_shared<SourceGroupCxxEmpty>(sourceGroupSettings));
}

TEST_F(SourceGroupFix, sourceGroupCxxCppEmptyGeneratesExpectedOutput) {
  EXPECT_CALL(*mMockedApplicationSettings, getHeaderSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path>{{"test/header/search/path"}}));
  EXPECT_CALL(*mMockedApplicationSettings, getFrameworkSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path>{{"test/framework/search/path"}}));

  const std::wstring projectName = L"cxx_cpp_empty";

  ProjectSettings projectSettings;
  projectSettings.setProjectFilePath(L"non_existent_project", getInputDirectoryPath(projectName));

  std::shared_ptr<SourceGroupSettingsCppEmpty> sourceGroupSettings = std::make_shared<SourceGroupSettingsCppEmpty>(
      "fake_id", &projectSettings);
  sourceGroupSettings->setSourcePaths({getInputDirectoryPath(projectName).concatenate(L"/src")});
  sourceGroupSettings->setSourceExtensions({L".cpp"});
  sourceGroupSettings->setExcludeFilterStrings({L"**/excluded/**"});
  sourceGroupSettings->setTargetOptionsEnabled(true);
  sourceGroupSettings->setTargetArch(L"test_arch");
  sourceGroupSettings->setTargetVendor(L"test_vendor");
  sourceGroupSettings->setTargetSys(L"test_sys");
  sourceGroupSettings->setTargetAbi(L"test_abi");
  sourceGroupSettings->setCppStandard(L"c++11");
  sourceGroupSettings->setHeaderSearchPaths({getInputDirectoryPath(projectName).concatenate(L"header_search/local")});
  sourceGroupSettings->setFrameworkSearchPaths({getInputDirectoryPath(projectName).concatenate(L"framework_search/local")});
  sourceGroupSettings->setCompilerFlags({L"-local-flag"});

  generateAndCompareExpectedOutput(projectName, std::make_shared<SourceGroupCxxEmpty>(sourceGroupSettings));
}

TEST_F(SourceGroupFix, sourceGroupCxxCdbGeneratesExpectedOutput) {
  EXPECT_CALL(*mMockedApplicationSettings, getHeaderSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path>{{"test/header/search/path"}}));
  EXPECT_CALL(*mMockedApplicationSettings, getFrameworkSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path>{{"test/framework/search/path"}}));

  const std::wstring projectName = L"cxx_cdb";

  ProjectSettings projectSettings;
  projectSettings.setProjectFilePath(L"non_existent_project", getInputDirectoryPath(projectName));

  std::shared_ptr<SourceGroupSettingsCxxCdb> sourceGroupSettings = std::make_shared<SourceGroupSettingsCxxCdb>(
      "fake_id", &projectSettings);
  sourceGroupSettings->setIndexedHeaderPaths({FilePath(L"test/indexed/header/path")});
  sourceGroupSettings->setCompilationDatabasePath(getInputDirectoryPath(projectName).concatenate(L"compile_commands.json"));
  sourceGroupSettings->setExcludeFilterStrings({L"**/excluded/**"});
  sourceGroupSettings->setHeaderSearchPaths({getInputDirectoryPath(projectName).concatenate(L"header_search/local")});
  sourceGroupSettings->setFrameworkSearchPaths({getInputDirectoryPath(projectName).concatenate(L"framework_search/local")});
  sourceGroupSettings->setCompilerFlags({L"-local-flag"});

  generateAndCompareExpectedOutput(projectName, std::make_shared<SourceGroupCxxCdb>(sourceGroupSettings));
}
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

#if BUILD_CXX_LANGUAGE_PACKAGE
TEST_F(SourceGroupFix, sourceGroupCxxCdbPrecompilesTheHeadersItsFilesShare) {
  EXPECT_CALL(*mMockedApplicationSettings, getHeaderSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path>{}));
  EXPECT_CALL(*mMockedApplicationSettings, getFrameworkSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path>{}));

  // A compilation database of enough files sharing their standard-library includes for an automatic
  // precompiled header to be worth building.
  const FilePath root =
      FilePath(std::filesystem::temp_directory_path().wstring()).concatenate(FilePath(L"sourcetrail_auto_pch_cdb"));
  std::filesystem::remove_all(root.str());
  std::filesystem::create_directories(root.str());

  std::string database = "[\n";
  for(int file = 0; file < 12; file++) {
    const FilePath sourcePath = root.getConcatenated(L"file" + std::to_wstring(file) + L".cpp");
    std::ofstream source(sourcePath.str());
    source << "#include <map>\n#include <string>\n#include <vector>\nint value" << file << " = " << file << ";\n";
    source.close();

    database += std::string(file == 0 ? "" : ",\n") + " {\"directory\": \"" + root.str() + "\", \"file\": \"" + sourcePath.str() +
        "\", \"command\": \"/usr/bin/clang++ -std=c++20 -c " + sourcePath.str() + "\"}";
  }
  database += "\n]\n";

  const FilePath databasePath = root.getConcatenated(L"compile_commands.json");
  std::ofstream databaseFile(databasePath.str());
  databaseFile << database;
  databaseFile.close();

  // Building a precompiled header needs the Clang builtin headers the app ships beside its binary.
  AppPath::setSharedDataDirectoryPath(FilePath(ST_SHARED_DATA_DIR));

  ProjectSettings projectSettings;
  projectSettings.setProjectFilePath(L"auto_pch_project", root);

  auto sourceGroupSettings = std::make_shared<SourceGroupSettingsCxxCdb>("fake_id", &projectSettings);
  sourceGroupSettings->setCompilationDatabasePath(databasePath);

  const SourceGroupCxxCdb sourceGroup(sourceGroupSettings);
  RefreshInfo info;
  info.filesToIndex = sourceGroup.getAllSourceFilePaths();
  const std::vector<std::shared_ptr<IndexerCommand>> indexerCommands = sourceGroup.getIndexerCommands(info);

  ASSERT_EQ(indexerCommands.size(), 12);

  const FilePath pchPath = sourceGroupSettings->getPchDependenciesDirectoryPath().getConcatenated(L"sourcetrail_auto_pch_0.pch");
  EXPECT_TRUE(pchPath.recheckExists()) << "no precompiled header at " << pchPath.str();

  for(const std::shared_ptr<IndexerCommand>& indexerCommand : indexerCommands) {
    const auto* cxxCommand = dynamic_cast<const IndexerCommandCxx*>(indexerCommand.get());
    ASSERT_NE(cxxCommand, nullptr);
    const std::vector<std::wstring> flags = cxxCommand->getCompilerFlags();
    EXPECT_NE(std::ranges::find(flags, L"-include-pch"), flags.end())
        << "command for " << indexerCommand->getSourceFilePath().str() << " does not use the precompiled header";
    EXPECT_NE(std::ranges::find(flags, pchPath.wstr()), flags.end());
  }

  // Presence of the flags is not the same as a parse accepting them: a cc1-only flag spelled
  // without -Xclang makes the driver reject the whole command line, which shows up as an indexing
  // error rather than a missing flag. So run one of the commands the way the indexer would.
  const auto firstCommand = std::dynamic_pointer_cast<IndexerCommandCxx>(indexerCommands.front());
  ASSERT_NE(firstCommand, nullptr);

  EXPECT_CALL(*mMockedApplicationSettings, getLoggingEnabled).WillRepeatedly(testing::Return(false));
  EXPECT_CALL(*mMockedApplicationSettings, getVerboseIndexerLoggingEnabled).WillRepeatedly(testing::Return(false));

  auto parsedStorage = std::make_shared<IntermediateStorage>();
  CxxParser parser(
      std::make_shared<ParserClientImpl>(parsedStorage.get()),
      std::make_shared<FileRegister>(
          firstCommand->getSourceFilePath(), std::set<FilePath>{firstCommand->getSourceFilePath()}, std::set<FilePathFilter>{}),
      std::make_shared<IndexerStateInfo>());
  parser.buildIndex(firstCommand);

  for(const StorageError& error : parsedStorage->getErrors()) {
    ADD_FAILURE() << "parsing with the precompiled header reported: " << utility::encodeToUtf8(error.message);
  }
}
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

#if BUILD_CXX_LANGUAGE_PACKAGE
TEST_F(SourceGroupFix, sourceGroupCxxEmptyBuildsThePrecompiledHeaderItsSettingsName) {
  EXPECT_CALL(*mMockedApplicationSettings, getHeaderSearchPathsExpanded)
      .WillRepeatedly(testing::Return(std::vector<std::filesystem::path>{}));
  EXPECT_CALL(*mMockedApplicationSettings, getFrameworkSearchPathsExpanded)
      .WillRepeatedly(testing::Return(std::vector<std::filesystem::path>{}));
  EXPECT_CALL(*mMockedApplicationSettings, getLoggingEnabled).WillRepeatedly(testing::Return(false));
  EXPECT_CALL(*mMockedApplicationSettings, getVerboseIndexerLoggingEnabled).WillRepeatedly(testing::Return(false));

  AppPath::setSharedDataDirectoryPath(FilePath(ST_SHARED_DATA_DIR));

  const FilePath root =
      FilePath(std::filesystem::temp_directory_path().wstring()).concatenate(FilePath(L"sourcetrail_manual_pch"));
  std::filesystem::remove_all(root.str());
  std::filesystem::create_directories(root.str());

  const FilePath headerPath = root.getConcatenated(L"stdafx.h");
  std::ofstream header(headerPath.str());
  header << "#pragma once\n#include <string>\n#include <vector>\n";
  header.close();

  const FilePath sourcePath = root.getConcatenated(L"main.cpp");
  std::ofstream source(sourcePath.str());
  source << "#include <string>\nstd::string name() { return \"x\"; }\n";
  source.close();

  ProjectSettings projectSettings;
  projectSettings.setProjectFilePath(L"manual_pch_project", root);

  auto sourceGroupSettings = std::make_shared<SourceGroupSettingsCppEmpty>("fake_id", &projectSettings);
  sourceGroupSettings->setSourcePaths({root});
  sourceGroupSettings->setSourceExtensions({L".cpp"});
  sourceGroupSettings->setCppStandard(L"c++20");
  sourceGroupSettings->setPchInputFilePathFilePath(headerPath);

  const SourceGroupCxxEmpty sourceGroup(sourceGroupSettings);

  auto storageProvider = std::make_shared<StorageProvider>();
  auto dialogView = std::make_shared<testing::NiceMock<MockedDialogView>>();
  sourceGroup.getPreIndexTask(storageProvider, dialogView)->update(std::make_shared<Blackboard>());

  const FilePath pchPath = sourceGroupSettings->getPchDependenciesDirectoryPath().getConcatenated(L"stdafx.pch");
  ASSERT_TRUE(pchPath.recheckExists()) << "no precompiled header at " << pchPath.str();

  // And the flags that make a parse read it have to be ones the driver accepts.
  RefreshInfo info;
  info.filesToIndex = sourceGroup.getAllSourceFilePaths();
  const std::vector<std::shared_ptr<IndexerCommand>> indexerCommands = sourceGroup.getIndexerCommands(info);
  ASSERT_EQ(indexerCommands.size(), 1);

  const auto command = std::dynamic_pointer_cast<IndexerCommandCxx>(indexerCommands.front());
  ASSERT_NE(command, nullptr);

  auto parsedStorage = std::make_shared<IntermediateStorage>();
  CxxParser parser(std::make_shared<ParserClientImpl>(parsedStorage.get()),
                   std::make_shared<FileRegister>(sourcePath, std::set<FilePath>{sourcePath}, std::set<FilePathFilter>{}),
                   std::make_shared<IndexerStateInfo>());
  parser.buildIndex(command);

  for(const StorageError& error : parsedStorage->getErrors()) {
    ADD_FAILURE() << "parsing with the precompiled header reported: " << utility::encodeToUtf8(error.message);
  }
}
#endif    // BUILD_CXX_LANGUAGE_PACKAGE

TEST_F(SourceGroupFix, sourceGroupCustomCommandGeneratesExpectedOutput) {
  const std::wstring projectName = L"custom_command";

  ProjectSettings projectSettings;
  projectSettings.setProjectFilePath(L"non_existent_project", getInputDirectoryPath(projectName));

  std::shared_ptr<SourceGroupSettingsCustomCommand> sourceGroupSettings = std::make_shared<SourceGroupSettingsCustomCommand>(
      "fake_id", &projectSettings);
  sourceGroupSettings->setCustomCommand(L"echo \"Hello World\"");
  sourceGroupSettings->setSourcePaths({getInputDirectoryPath(projectName).concatenate(L"/src")});
  sourceGroupSettings->setSourceExtensions({L".txt"});
  sourceGroupSettings->setExcludeFilterStrings({L"**/excluded/**"});

  generateAndCompareExpectedOutput(projectName, std::make_shared<SourceGroupCustomCommand>(sourceGroupSettings));
}

TEST_F(SourceGroupFix, canDestroyApplicationInstance) {
  EXPECT_CALL(*mMockedMessageQueue, stopMessageLoop).WillOnce(testing::Return());

  Application::destroyInstance();
  EXPECT_TRUE(0 == Application::getInstance().use_count());
}
