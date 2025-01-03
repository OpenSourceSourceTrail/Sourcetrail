#include <cstdlib>
#include <filesystem>

#include <fmt/format.h>

#include <gtest/gtest.h>

#include "FilePath.h"
#include "utilityString.h"

namespace fs = std::filesystem;

#ifndef __FILE_NAME__
#  define __FILE_NAME__ fs::path(__FILE__).filename().string().c_str()
#endif

TEST(FilePathTest, CallCopyConstructor) {
  // Given: FilePath inst
  FilePath inst{__FILE__};

  // When: calling FilePath copy constructor
  auto newInst = inst;

  // Then: inst equals to newInst
  EXPECT_EQ(inst, newInst);
}

TEST(FilePathTest, CallMoveConstructor) {
  // Given: FilePath inst
  FilePath inst{__FILE__};

  // When: calling FilePath move constructor
  auto newInst = std::move(inst);

// Then: inst equals to newInst
#ifdef _WIN32
  EXPECT_EQ(newInst.str(), utility::replace(__FILE__, "\\", "/"));
#else
  EXPECT_EQ(newInst.str(), __FILE__);
#endif
}

TEST(FilePathTest, CallCustomConstructor) {
  // Given: expectedFilePath
  FilePath expectedFilePath{__FILE__};

  // When: call FilePath Constructor
  FilePath inst{expectedFilePath.fileName(), expectedFilePath.getParentDirectory().wstr()};

  // Then: inst equals to expectedFilePath
  EXPECT_EQ(inst.str(), expectedFilePath.makeAbsolute().str());
}

TEST(FilePathTest, GetInternalPathVariable) {
  // Given: FilePath instance, expectedInternalPath
  FilePath inst{__FILE__};
  fs::path expectedInternalPath{__FILE__};

  // When: calling getPath
  auto internalPath = inst.getPath();

  // Then: internalPath is equal to expectedInternalPath
  EXPECT_EQ(expectedInternalPath, internalPath);
}

TEST(FilePathTest, CreateEmptyFilePathInstance) {
  // Given: access to FilePath

  // When: calling EmptyFilePath
  auto instance = FilePath::EmptyFilePath();

  // Then: instance.empty() is true as path is empty
  EXPECT_TRUE(instance.empty());
}

TEST(FilePathTest, NonEmptyFilePath) {
  // Given: Non empty FilePath instance
  FilePath inst{__FILE__};

  // When: calling empty
  auto isEmpty = inst.empty();

  // Then: isEmpty is false as path not empty
  EXPECT_FALSE(isEmpty);
}

TEST(FilePathTest, FilePathExists) {
  // Given: Existing FilePath instance
  FilePath inst{__FILE__};

  // When: calling exists
  auto isExist = inst.exists();

  // Then: isExist is true as path already exist
  EXPECT_TRUE(isExist);
}

TEST(FilePathTest, FilePathDoesnotExist) {
// Given: Non Existing FilePath instance
#ifdef _WIN32
  FilePath inst{L"C:\\Documents\\Newsletters\\Summer2018.pdf"};
#else
  FilePath inst{"/home/hamada/by7b/mayada/3sk.img"};
#endif

  // When: calling exists
  auto isExist = inst.exists();

  // Then: isExist is false as path doesn't exist
  EXPECT_FALSE(isExist);
}

TEST(FilePathTest, RecheckIfFilePathExist) {
  // Given: Existing FilePath instance
  FilePath inst{__FILE__};

  // When: calling recheckExists
  auto isExist = inst.recheckExists();

  // Then: isExist is true as path already exist
  EXPECT_TRUE(isExist);
}

TEST(FilePathTest, IsNotDirectory) {
  // Given: FilePath instance
  FilePath inst{__FILE__};

  // When: calling isDirectory
  auto isDir = inst.isDirectory();

  // Then: isDir is false as it's file path not directory
  EXPECT_FALSE(isDir);
}

TEST(FilePathTest, IsDirectory) {
  // Given: FilePath instance
  FilePath inst{LIB_TEST_ROOT_DIR};

  // When: calling isDirectory
  auto isDir = inst.isDirectory();

  // Then: isDir is true as it's directory
  EXPECT_TRUE(isDir);
}

TEST(FilePathTest, AbsoluteFilePath) {
  // Given: Absolute FilePath instance
  FilePath inst{__FILE__};

  // When: calling isAbsolute
  auto isAbsolute = inst.isAbsolute();

  // Then: isAbsolute is true as path is already absolute
  EXPECT_TRUE(isAbsolute);
}

TEST(FilePathTest, RelativeFilePath) {
  // Given: Relative FilePath instance
  FilePath inst{__FILE__ + SOURCE_PATH_PREFIX_LEN};

  // When: calling isAbsolute
  auto isAbsolute = inst.isAbsolute();

  // Then: isAbsolute is false as path is relative to root directory
  EXPECT_FALSE(isAbsolute);
}

TEST(FilePathTest, ValidFilePath) {
  // Given: FilePath instance
  FilePath inst{__FILE__};

  // When: calling isValid
  auto isValid = inst.isValid();

  // Then: isValid is true
  EXPECT_TRUE(isValid);
}

TEST(FilePathTest, InvalidFilePath) {
  // Given: FilePath instance
#ifdef _WIN32
  FilePath inst{L"C:\\workspace\\pis\\report\\2020-07-20-14:45:08_report.html"};
#else
  FilePath inst{"/Muhammad\\Hussein/tmp/file.txt"};
#endif

  // When: calling isValid
  auto isValid = inst.isValid();

  // Then: isValid is false
  EXPECT_FALSE(isValid);
}

TEST(FilePathTest, GetParentPath) {
  // Given: FilePath instance, expectedParentDir (parent directory of current file)
  FilePath inst{__FILE__};
  FilePath expectedParentDir(LIB_TEST_ROOT_DIR);

  // When: calling getParentDirectory
  auto parentDir = inst.getParentDirectory();

  // Then: parentDir is equal to expectedParentDir
  EXPECT_EQ(expectedParentDir, parentDir);
}

TEST(FilePathTest, MakeAbsolutePath) {
  // Given: FilePath inst, expectedAbsoluteFilePath
  FilePath inst{__FILE_NAME__};
  FilePath expectedAbsoluteFilePath{std::filesystem::current_path().append(__FILE_NAME__).string()};

  // When: calling makeAbsolute
  auto absolutePath = inst.makeAbsolute();

  // Then: absolutePath equals to expectedAbsoluteFilePath
  EXPECT_EQ(absolutePath, expectedAbsoluteFilePath);
}

TEST(FilePathTest, GetAbsolutePath) {
  // Given: FilePath inst, expectedAbsoluteFilePath
  FilePath inst{__FILE_NAME__};
  FilePath expectedAbsoluteFilePath{std::filesystem::current_path().append(__FILE_NAME__).string()};

  // When: calling getAbsolute
  auto absolutePath = inst.getAbsolute();

  // Then: absolutePath equals to expectedAbsoluteFilePath
  EXPECT_EQ(absolutePath, expectedAbsoluteFilePath);
}

TEST(FilePathTest, MakeCanonicalPath) {
  // Given: FilePath inst, expectedFilePath
  auto inst = FilePath{LIB_TEST_ROOT_DIR}.concatenate(L"../tests/FilePathTestSuite.cpp");
  auto expectedFilePath = FilePath{__FILE__};

  // When: calling makeCanonical
  auto canonicalFilePath = inst.makeCanonical();

  // Then: canonicalFilePath equals to expectedFilePath
  EXPECT_EQ(canonicalFilePath, expectedFilePath);
}

TEST(FilePathTest, GetCanonicalPath) {
  // Given: FilePath inst, expectedFilePath
  auto inst = FilePath{LIB_TEST_ROOT_DIR}.concatenate(L"../tests/FilePathTestSuite.cpp");
  auto expectedFilePath = FilePath{__FILE__};

  // When: calling getCanonical
  auto canonicalFilePath = inst.getCanonical();

  // Then: canonicalFilePath equals to expectedFilePath
  EXPECT_EQ(canonicalFilePath, expectedFilePath);
}

TEST(FilePathTest, MakeRelativePath) {
  // Given: FilePath inst, expectedFilePath
  auto inst = FilePath{__FILE__};
  auto expectedFilePath = FilePath{__FILE_NAME__};
  auto currentDirPath = FilePath("./");

  // When: calling makeRelativeTo
  auto relativeFilePath = inst.makeRelativeTo(FilePath{LIB_TEST_ROOT_DIR});

  // Then: relativeFilePath equals to expectedFilePath
  EXPECT_EQ(relativeFilePath, expectedFilePath);

  // When: calling makeRelativeTo
  relativeFilePath = inst.makeRelativeTo(inst);

  // Then: relativeFilePath equals to currentDirPath
  EXPECT_EQ(relativeFilePath, currentDirPath);

  inst = FilePath{ROOT_DIR};
  // When: calling makeRelativeTo
  relativeFilePath = inst.makeRelativeTo(FilePath{LIB_TEST_ROOT_DIR});

  // Then: relativeFilePath equals to ../../../
  EXPECT_EQ(relativeFilePath, FilePath{"../../../"});
}

TEST(FilePathTest, GetRelativePath) {
  // Given: FilePath inst, expectedFilePath
  auto inst = FilePath{__FILE__};
  auto expectedFilePath = FilePath{__FILE_NAME__};

  // When: calling getRelativeTo
  auto relativeFilePath = inst.getRelativeTo(FilePath{LIB_TEST_ROOT_DIR});

  // Then: relativeFilePath equals to expectedFilePath
  EXPECT_EQ(relativeFilePath, expectedFilePath);
}

TEST(FilePathTest, ConcatenateFilePath) {
  // Given: FilePath instance, fileName(FilePath), expectedFullPath
  FilePath inst{LIB_TEST_ROOT_DIR};
  FilePath fileName{L"FilePathTestSuite.cpp"};
  FilePath expectedFullPath{__FILE__};

  // When: calling concatenate
  auto fullPath = inst.concatenate(fileName);

  // Then: fullPath is equal to expectedFullPath
  EXPECT_EQ(fullPath, expectedFullPath);
}

TEST(FilePathTest, GetConcatenatedFilePath) {
  // Given: FilePath instance, fileName(FilePath), expectedFullPath
  FilePath inst{LIB_TEST_ROOT_DIR};
  FilePath fileName{L"FilePathTestSuite.cpp"};
  FilePath expectedFullPath{__FILE__};

  // When: calling getConcatenated
  auto fullPath = inst.getConcatenated(fileName);

  // Then: fullPath is equal to expectedFullPath
  EXPECT_EQ(fullPath, expectedFullPath);
}

TEST(FilePathTest, ConcatenateWStr) {
  // Given: FilePath instance, fileName (wstring), expectedFullPath
  FilePath inst{LIB_TEST_ROOT_DIR};
  std::wstring fileName = L"FilePathTestSuite.cpp";
  FilePath expectedFullPath{__FILE__};

  // When: calling concatenate
  auto fullPath = inst.concatenate(fileName);

  // Then: fullPath is equal to expectedFullPath
  EXPECT_EQ(fullPath, expectedFullPath);
}

TEST(FilePathTest, GetConcatenatedWStr) {
  // Given: FilePath instance, fileName (wstring), expectedFullPath
  FilePath inst{LIB_TEST_ROOT_DIR};
  std::wstring fileName = L"FilePathTestSuite.cpp";
  FilePath expectedFullPath{__FILE__};

  // When: calling getConcatenated
  auto fullPath = inst.getConcatenated(fileName);

  // Then: fullPath is equal to expectedFullPath
  EXPECT_EQ(fullPath, expectedFullPath);
}

TEST(FilePathTest, GetFilePathInLoweCase) {
  // Given: 2 FilePath instance
  FilePath inst{__FILE__};
  FilePath expectedFilePath{utility::toLowerCase(__FILE__)};

  // When: calling getLowerCase
  auto newFilePath = inst.getLowerCase();

  // Then: newFilePath is equal to expectedFilePath
  EXPECT_EQ(newFilePath, expectedFilePath);
}

TEST(FilePathTest, SplitByEnvironmentVariablesToFilePath) {
// Given: FilePath inst
#ifdef _WIN32
  _putenv(fmt::format("TEST_FILE={}", __FILE_NAME__).c_str());
#else
  setenv("TEST_FILE", __FILE_NAME__, 1);
#endif

  auto inst = FilePath(LIB_TEST_ROOT_DIR).concatenate(L"${TEST_FILE}");

  // When: calling expandEnvironmentVariables
  auto filePaths = inst.expandEnvironmentVariables();

  // Then: filePaths size is 1
  EXPECT_EQ(filePaths.size(), 1);
  EXPECT_EQ(filePaths[0], FilePath(__FILE__));
}

TEST(FilePathTest, SplitByEnvironmentVariablesToFilePathFailed) {
  // Given: FilePath inst
  auto inst = FilePath(LIB_TEST_ROOT_DIR).concatenate(L"${TEST_FILE}");

  // When: calling expandEnvironmentVariables
  auto filePaths = inst.expandEnvironmentVariables();

  // Then: filePaths size is 0
  EXPECT_EQ(filePaths.size(), 0);
}

TEST(FilePathTest, SplitByEnvironmentVariablesToStl) {
// Given: FilePath inst, env variable(TEST_FILE)
#ifdef _WIN32
  _putenv(fmt::format("TEST_FILE={}", __FILE_NAME__).c_str());
#else
  setenv("TEST_FILE", __FILE_NAME__, 1);
#endif

  auto inst = FilePath(LIB_TEST_ROOT_DIR).concatenate(L"${TEST_FILE}");

  // When: calling expandEnvironmentVariablesStl
  auto filePaths = inst.expandEnvironmentVariablesStl();

  // Then: filePaths size is 1
  EXPECT_EQ(filePaths.size(), 1);
  EXPECT_EQ(filePaths[0], std::filesystem::path(__FILE__));
}

TEST(FilePathTest, SplitByEnvironmentVariablesToStlFailed) {
  // Given: FilePath inst
  auto inst = FilePath(LIB_TEST_ROOT_DIR).concatenate(L"${TEST_FILE}");

  // When: calling expandEnvironmentVariablesStl
  auto filePaths = inst.expandEnvironmentVariablesStl();

  // Then: filePaths size is 0
  EXPECT_EQ(filePaths.size(), 0);
}

TEST(FilePathTest, ContainsFilePath) {
  // Given: 2 FilePath instance
  FilePath rootTestDir{LIB_TEST_ROOT_DIR};
  FilePath currentFile{__FILE__};
  FilePath rootDir{ROOT_DIR};

  // When: calling contains
  auto contained = rootTestDir.contains(currentFile);

  // Then: contained is true
  EXPECT_TRUE(contained);

  // When: calling contains
  contained = currentFile.contains(FilePath{__FILE_NAME__});

  // Then: contained is false as currentFile is not a directory
  EXPECT_FALSE(contained);

  // When: calling contains
  contained = rootTestDir.contains(FilePath{ROOT_DIR}.concatenate(L"scripts"));

  // Then: contained is false
  EXPECT_FALSE(contained);

  // When: calling contains
  contained = rootTestDir.concatenate(L".").contains(rootDir);

  // Then: contained is false
  EXPECT_FALSE(contained);
}

TEST(FilePathTest, GetPathInStrFormat) {
  // Given: FilePath instance
  FilePath inst{__FILE__};

  // When: calling str
  auto path = inst.str();

// Then: path is equal to current file path
#ifdef _WIN32
  EXPECT_EQ(path, utility::replace(__FILE__, "\\", "/"));
#else
  EXPECT_EQ(path, __FILE__);
#endif
}

TEST(FilePathTest, GetPathInWStrFormat) {
  // Given: FilePath instance, expectedPath (current file path in wstring format)
  FilePath inst{__FILE__};
  std::wstring expectedPath{__FILE__, __FILE__ + sizeof(__FILE__) - 1};

  // When: calling wstr
  auto path = inst.wstr();

// Then: path is equal to expectedPath
#ifdef _WIN32
  EXPECT_EQ(path, utility::replace(expectedPath, L"\\", L"/"));
#else
  EXPECT_EQ(path, expectedPath);
#endif
}

TEST(FilePathTest, GetFileName) {
  // Given: FilePath instance, expectedFileName
  FilePath inst{__FILE__};
  auto expectedFileName = L"FilePathTestSuite.cpp";

  // When: calling fileName
  auto fileName = inst.fileName();

  // Then: fileName is equal to expectedFileName
  EXPECT_EQ(fileName, expectedFileName);
}

TEST(FilePathTest, GetFileExtension) {
  // Given: FilePath instance, expectedFileExtension
  FilePath inst{__FILE__};
  auto expectedFileExtension = L".cpp";

  // When: calling extension
  auto fileExtension = inst.extension();

  // Then: fileExtension is equal to expectedFileExtension
  EXPECT_EQ(fileExtension, expectedFileExtension);
}

TEST(FilePathTest, RemoveFilePathExtension) {
  // Given: FilePath instance, exptectedFilePath (FilePath without extension)
  FilePath inst{__FILE__};
  std::string path{__FILE__};
  FilePath exptectedFilePath{path.substr(0, path.rfind('.'))};

  // When: calling withoutExtension
  auto pathWithoutExtension = inst.withoutExtension();

  // Then: pathWithoutExtension is equal to exptectedFilePath
  EXPECT_EQ(pathWithoutExtension, exptectedFilePath);
}

TEST(FilePathTest, ReplaceFilePathExtension) {
  // Given: FilePath instance, exptectedFilePath (FilePath with .cxx extension)
  FilePath inst{__FILE__};
  std::string path{__FILE__};
  FilePath exptectedFilePath{path.substr(0, path.rfind('.')) + ".cxx"};

  // When: calling replaceExtension
  auto newPathWithCxxExtension = inst.replaceExtension(L".cxx");

  // Then: newPathWithCxxExtension is equal to exptectedFilePath
  EXPECT_EQ(newPathWithCxxExtension, exptectedFilePath);
}

TEST(FilePathTest, CheckFilePathExtension) {
  // Given: FilePath instance
  FilePath inst{__FILE__};

  // When: calling hasExtension
  auto hasExtension = inst.hasExtension({L".c", L".py", L".cxx"});

  // Then: hasExtension is false no matching extension
  EXPECT_FALSE(hasExtension);

  // When: calling hasExtension
  hasExtension = inst.hasExtension({L".c", L".py", L".cpp"});

  // Then: hasExtension is true
  EXPECT_TRUE(hasExtension);
}

TEST(FilePathTest, CallCopyAssignmentOperator) {
  // Given: FilePath inst
  FilePath inst{__FILE__};

  // When: calling FilePath copy assignment operator
  FilePath newInst;
  newInst = inst;

  // Then: inst equals to copy_inst
  EXPECT_EQ(inst, newInst);
}

TEST(FilePathTest, CallMoveAssignmentOperator) {
  // Given: FilePath inst
  FilePath inst{__FILE__};

  // When: calling FilePath move assignment operator
  FilePath newInst;
  newInst = std::move(inst);

// Then: inst equals to newInst
#ifdef _WIN32
  EXPECT_EQ(newInst.str(), utility::replace(__FILE__, "\\", "/"));
#else
  EXPECT_EQ(newInst.str(), __FILE__);
#endif
}

TEST(FilePathTest, NotEqualFilePaths) {
  // Given: 2 FilePaths
  FilePath path1{__FILE__};
  FilePath path2{LIB_TEST_ROOT_DIR};

  // When: calling != operator
  auto notEqual = path1 != path2;

  // Then: notEqual is true
  EXPECT_TRUE(notEqual);
}

TEST(FilePathTest, LessThanComparison) {
  // Given: 2 FilePaths
  FilePath path1{__FILE__};
  FilePath path2{LIB_TEST_ROOT_DIR};

  // When: calling < operator
  auto lessThan = path2 < path1;

  // Then: lessThan is true
  EXPECT_TRUE(lessThan);
}
