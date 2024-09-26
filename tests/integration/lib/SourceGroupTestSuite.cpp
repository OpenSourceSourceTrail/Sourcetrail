#include <fstream>

#include <gtest/gtest.h>

#include "AppPath.h"
#include "Application.h"
#include "ApplicationSettings.h"
#include "FileSystem.h"
#include "IApplicationSettings.hpp"
#include "IndexerCommandCustom.h"
#include "ProjectSettings.h"
#include "SourceGroupCustomCommand.h"
#include "SourceGroupSettingsCustomCommand.h"
#include "TextAccess.h"
#include "Version.h"
#include "language_packages.h"
#include "utilityPathDetection.h"
#include "utilityString.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#  include "../../../src/lib/tests/mocks/MockedApplicationSetting.hpp"
#  include "IndexerCommandCxx.h"
#  include "SourceGroupCxxCdb.h"
#  include "SourceGroupCxxCodeblocks.h"
#  include "SourceGroupCxxEmpty.h"
#  include "SourceGroupSettingsCEmpty.h"
#  include "SourceGroupSettingsCppEmpty.h"
#  include "SourceGroupSettingsCxxCdb.h"
#  include "SourceGroupSettingsCxxCodeblocks.h"
#  include "utilityFile.h"
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
    IApplicationSettings::setInstance(mMocked);
  }

  void TearDown() override {
    IApplicationSettings::setInstance(nullptr);
    mMocked.reset();
  }

  std::shared_ptr<testing::StrictMock<MockedApplicationSettings>> mMocked =
      std::make_shared<testing::StrictMock<MockedApplicationSettings>>();
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
  EXPECT_CALL(*mMocked, getHeaderSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path> {{"test/header/search/path"}}));
  EXPECT_CALL(*mMocked, getFrameworkSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path> {{"test/framework/search/path"}}));

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
  EXPECT_CALL(*mMocked, getHeaderSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path> {{"test/header/search/path"}}));
  EXPECT_CALL(*mMocked, getFrameworkSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path> {{"test/framework/search/path"}}));

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

TEST_F(SourceGroupFix, sourceGroupCxxCodeblocksGeneratesExpectedOutput) {
  EXPECT_CALL(*mMocked, getHeaderSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path> {{"test/header/search/path"}}));
  EXPECT_CALL(*mMocked, getFrameworkSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path> {{"test/framework/search/path"}}));

  const std::wstring projectName = L"cxx_codeblocks";
  const FilePath cbpPath = getInputDirectoryPath(projectName).concatenate(L"project.cbp");
  const FilePath sourceCbpPath = getInputDirectoryPath(projectName).concatenate(L"project.cbp.in");

  FileSystem::remove(cbpPath);

  {
    std::ofstream fileStream;
    fileStream.open(cbpPath.str(), std::ios::app);
    fileStream << utility::replace(TextAccess::createFromFile(sourceCbpPath)->getText(),
                                   "<source_path>",
                                   getInputDirectoryPath(projectName).concatenate(L"src").getAbsolute().str());
    fileStream.close();
  }

  ProjectSettings projectSettings;
  projectSettings.setProjectFilePath(L"non_existent_project", getInputDirectoryPath(projectName));

  std::shared_ptr<SourceGroupSettingsCxxCodeblocks> sourceGroupSettings = std::make_shared<SourceGroupSettingsCxxCodeblocks>(
      "fake_id", &projectSettings);
  sourceGroupSettings->setCodeblocksProjectPath(cbpPath);
  sourceGroupSettings->setCppStandard(L"c++11");
  sourceGroupSettings->setCStandard(L"c11");
  sourceGroupSettings->setExcludeFilterStrings({L"**/excluded/**"});
  sourceGroupSettings->setIndexedHeaderPaths({FilePath(L"test/indexed/header/path")});
  sourceGroupSettings->setSourceExtensions({L".cpp", L".c"});
  sourceGroupSettings->setHeaderSearchPaths({getInputDirectoryPath(projectName).concatenate(L"header_search/local")});
  sourceGroupSettings->setFrameworkSearchPaths({getInputDirectoryPath(projectName).concatenate(L"framework_search/local")});
  sourceGroupSettings->setCompilerFlags({L"-local-flag"});

  generateAndCompareExpectedOutput(projectName, std::make_shared<SourceGroupCxxCodeblocks>(sourceGroupSettings));

  FileSystem::remove(cbpPath);
}

TEST_F(SourceGroupFix, sourceGroupCxxCdbGeneratesExpectedOutput) {
  EXPECT_CALL(*mMocked, getHeaderSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path> {{"test/header/search/path"}}));
  EXPECT_CALL(*mMocked, getFrameworkSearchPathsExpanded)
      .WillOnce(testing::Return(std::vector<std::filesystem::path> {{"test/framework/search/path"}}));

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
  Application::destroyInstance();
  EXPECT_TRUE(0 == Application::getInstance().use_count());
}
