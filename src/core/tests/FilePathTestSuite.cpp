#include <gtest/gtest.h>

#include "FilePath.h"

TEST(FilePath, gets_created_empty) {
  const FilePath path;

  EXPECT_TRUE(path.wstr().empty());
}

TEST(FilePath, gets_created_with_string) {
  const std::wstring str(L"data/FilePathTestSuite/main.cpp");
  const FilePath path(str);

  EXPECT_TRUE(path.wstr() == str);
}

TEST(FilePath, gets_created_other_file_path) {
  const FilePath path(L"data/FilePathTestSuite/main.cpp");
  const FilePath path2(path);    // NOLINT(performance-unnecessary-copy-initialization)

  EXPECT_TRUE(path == path2);
}

TEST(FilePath, empty) {
  const FilePath path1(L"data/FilePathTestSuite/a.cpp");
  const FilePath path2;

  EXPECT_TRUE(!path1.empty());
  EXPECT_TRUE(path2.empty());
}

TEST(FilePath, exists) {
  const FilePath path(L"data/FilePathTestSuite/a.cpp");

  EXPECT_TRUE(path.exists());
}

TEST(FilePath, not_exists) {
  const FilePath path(L"data/FilePathTestSuite/a.h");

  EXPECT_TRUE(!path.exists());
}

TEST(FilePath, is_directory) {
  const FilePath path(L"data/FilePathTestSuite/a.cpp");

  EXPECT_TRUE(!path.isDirectory());
  EXPECT_TRUE(path.getParentDirectory().isDirectory());
}

TEST(FilePath, empty_file_path_has_empty_parent_directory) {
  const FilePath path;

  EXPECT_TRUE(path.empty());
  EXPECT_TRUE(path.getParentDirectory().empty());
}

TEST(FilePath, without_parent_has_empty_parent_directory) {
  const FilePath path(L"a.cpp");

  EXPECT_TRUE(path.getParentDirectory().empty());
}

TEST(FilePath, is_absolute) {
  const FilePath path(L"data/FilePathTestSuite/a.cpp");

  EXPECT_TRUE(!path.isAbsolute());
  EXPECT_TRUE(path.getAbsolute().isAbsolute());
}

TEST(FilePath, parent_directory) {
  const FilePath path(L"data/FilePathTestSuite/a.cpp");

  EXPECT_TRUE(path.getParentDirectory().wstr() == L"data/FilePathTestSuite");
  EXPECT_TRUE(path.getParentDirectory().getParentDirectory().wstr() == L"data");
}

TEST(FilePath, relative_to_other_path) {
  const FilePath pathA(L"data/FilePathTestSuite/a.cpp");
  const FilePath pathB(L"data/FilePathTestSuite/test/c.h");

  EXPECT_TRUE(pathA.getRelativeTo(pathB).wstr() == L"../a.cpp");
  EXPECT_TRUE(pathB.getRelativeTo(pathA).wstr() == L"test/c.h");
}

TEST(FilePath, relative_to_other_directory) {
  const FilePath pathA(L"data/FilePathTestSuite/a.cpp");
  const FilePath pathB(L"data/FilePathTestSuite/test");

  EXPECT_TRUE(pathA.getRelativeTo(pathB).wstr() == L"../a.cpp");
}

TEST(FilePath, relative_to_same_directory) {
  const FilePath pathA(L"data/FilePathTestSuite/test");

  EXPECT_TRUE(pathA.getRelativeTo(pathA).wstr() == L"./");
}

TEST(FilePath, file_name) {
  const FilePath path(L"data/FilePathTestSuite/abc.h");

  EXPECT_TRUE(path.fileName() == L"abc.h");
}

TEST(FilePath, extension) {
  const FilePath path(L"data/FilePathTestSuite/a.h");

  EXPECT_TRUE(path.extension() == L".h");
}

TEST(FilePath, without_extension) {
  const FilePath path(L"data/FilePathTestSuite/a.h");

  EXPECT_TRUE(path.withoutExtension() == FilePath(L"data/FilePathTestSuite/a"));
}

TEST(FilePath, has_extension) {
  std::vector<std::wstring> extensions;
  extensions.emplace_back(L".h");
  extensions.emplace_back(L".cpp");
  extensions.emplace_back(L".cc");

  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/a.h").hasExtension(extensions));
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/b.cpp").hasExtension(extensions));
  EXPECT_TRUE(!FilePath(L"data/FilePathTestSuite/a.m").hasExtension(extensions));
}

TEST(FilePath, equals_file_with_different_relative_paths) {
  const FilePath path1(L"data/FilePathTestSuite/a.cpp");
  const FilePath path2(L"data/../data/FilePathTestSuite/./a.cpp");

  EXPECT_TRUE(path1 == path2);
}

TEST(FilePath, equals_relative_and_absolute_paths) {
  const FilePath path1(L"data/FilePathTestSuite/a.cpp");
  const FilePath path2 = path1.getAbsolute();

  EXPECT_TRUE(path1 == path2);
}

TEST(FilePath, equals_absolute_and_canonical_paths) {
  const FilePath path(L"data/../data/FilePathTestSuite/./a.cpp");

  EXPECT_TRUE(path.getAbsolute() == path.getCanonical());
}

#if 0
TEST(FilePath, canonical_removes_symlinks) {
#  ifndef _WIN32
  const FilePath pathA(L"data/FilePathTestSuite/parent/target/d.cpp");
  const FilePath pathB(L"data/FilePathTestSuite/target/d.cpp");

  EXPECT_TRUE(pathB.getAbsolute() == pathA.getCanonical());
#  endif
}
#endif

TEST(FilePath, compares_paths_with_posix_and_windows_format) {
#ifdef _WIN32
  const FilePath pathB(L"data/FilePathTestSuite/b.cc");
  const FilePath pathB2(L"data\\FilePathTestSuite\\b.cc");

  EXPECT_TRUE(pathB == pathB2);
#endif
}

TEST(FilePath, differs_for_different_existing_files) {
  const FilePath pathA(L"data/FilePathTestSuite/a.cpp");
  const FilePath pathB(L"data/FilePathTestSuite/b.cc");

  EXPECT_TRUE(pathA != pathB);
}

TEST(FilePath, differs_for_different_nonexistent_files) {
  const FilePath pathA(L"data/FilePathTestSuite/a.h");
  const FilePath pathB(L"data/FilePathTestSuite/b.c");

  EXPECT_TRUE(pathA != pathB);
}

TEST(FilePath, differs_for_existing_and_nonexistent_files) {
  const FilePath pathA(L"data/FilePathTestSuite/a.h");
  const FilePath pathB(L"data/FilePathTestSuite/b.cc");

  EXPECT_TRUE(pathA != pathB);
}

TEST(FilePath, validForAbsoluteAndRelativeExistingFilesAndDirectoriesPaths) {
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/a.cpp").isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/a.cpp").makeAbsolute().isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/with space/s.srctrlprj").isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/with space/s.srctrlprj").makeAbsolute().isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite").isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite").makeAbsolute().isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/container.app").isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/container.app").makeAbsolute().isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/container.app/b.txt").isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/container.app/b.txt").makeAbsolute().isValid());
}

TEST(FilePath, validForAbsoluteAndRelativeNonExistingFilesAndDirectoriesPaths) {
  EXPECT_TRUE(FilePath(L"data/non-existing-file.cpp").isValid());
  EXPECT_TRUE(FilePath(L"data/non-existing-file.cpp").makeAbsolute().isValid());
  EXPECT_TRUE(FilePath(L"data/non-existing-dir").isValid());
  EXPECT_TRUE(FilePath(L"data/non-existing-dir").makeAbsolute().isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/container.app/c.txt").isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/container.app/c.txt").makeAbsolute().isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/also space").isValid());
  EXPECT_TRUE(FilePath(L"data/FilePathTestSuite/also space").makeAbsolute().isValid());
}

TEST(FilePath, invalidForAbsoluteAndRelativePathsWithInvalidCharacters) {
  EXPECT_TRUE(!FilePath(L"data/non-exis\"ting-file.cpp").isValid());
  EXPECT_TRUE(!FilePath(L"data/non-exis\"ting-file.cpp").makeAbsolute().isValid());
  EXPECT_TRUE(!FilePath(L"data/non-exis|ting-dir").isValid());
  EXPECT_TRUE(!FilePath(L"data/non-exis|ting-dir").makeAbsolute().isValid());
  EXPECT_TRUE(!FilePath(L"data/FilePathTestSuite/container:app").isValid());
  EXPECT_TRUE(!FilePath(L"data/FilePathTestSuite/container:app").makeAbsolute().isValid());
}
