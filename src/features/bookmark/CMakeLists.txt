# ${PROJECT_SOURCE_DIR}/src/features/bookmark/CMakeLists.txt
#
# The bookmark feature. This is the reference shape for every feature directory -- copy it.
#
# A feature owns all three tiers of one slice of the application, but does NOT get its own targets:
# it contributes sources to the three tier targets that already exist. Feature is the directory
# axis, tier is the target axis, and a file's tier is readable from its path:
#
#   domain/    -> Sourcetrail_lib          entities, value types, POD storage rows
#   logic/     -> Sourcetrail_lib          controller, abstract view interfaces, algorithms
#   messages/  -> Sourcetrail_lib          this feature's Message* headers (header-only)
#   data/      -> Sourcetrail_lib_engine   SQLite storage; NEVER reachable from ui/
#   ui/        -> Sourcetrail_lib_gui      Qt widgets, windows, concrete views
#   tests/                                 this feature's suites and view mocks
#
# The one-way arrows (ui -> logic -> domain, ui never -> data) are enforced by
# assert_no_transitive_link() in src/lib/lib_gui/CMakeLists.txt, not by convention.
#
# INCLUDES: src/features is the include root, added once in src/features/CMakeLists.txt, so every
# include reads "<feature>/<tier>/<File>.h" -- e.g. "bookmark/domain/Bookmark.h". Feature-first and
# tier-second, so a wrong-tier include is visible at the include line without opening anything.
#
# CTEST: a feature registers its own suites here, with the prefix of the tier under test, so the
# existing ctest buckets keep working (`ctest -R "unittests\.lib\."` still selects Logic tests).

target_sources(
  Sourcetrail_lib
  PRIVATE domain/Bookmark.cpp
          domain/Bookmark.h
          domain/BookmarkCategory.cpp
          domain/BookmarkCategory.h
          domain/EdgeBookmark.cpp
          domain/EdgeBookmark.h
          domain/NodeBookmark.cpp
          domain/NodeBookmark.h
          domain/StorageBookmark.h
          domain/StorageBookmarkCategory.h
          domain/StorageBookmarkedEdge.h
          domain/StorageBookmarkedNode.h
          logic/BookmarkButtonsView.cpp
          logic/BookmarkButtonsView.h
          logic/BookmarkController.cpp
          logic/BookmarkController.h
          logic/BookmarkView.cpp
          logic/BookmarkView.h
          messages/MessageBookmarkActivate.h
          messages/MessageBookmarkBrowse.h
          messages/MessageBookmarkButtonState.h
          messages/MessageBookmarkCreate.h
          messages/MessageBookmarkDelete.h
          messages/MessageBookmarkEdit.h
          messages/MessageBookmarkUpdate.hpp)

target_sources(Sourcetrail_lib_engine PRIVATE data/SqliteBookmarkStorage.cpp data/SqliteBookmarkStorage.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtBookmark.cpp
          ui/QtBookmark.h
          ui/QtBookmarkBrowser.cpp
          ui/QtBookmarkBrowser.h
          ui/QtBookmarkButtonsView.cpp
          ui/QtBookmarkButtonsView.h
          ui/QtBookmarkCategory.cpp
          ui/QtBookmarkCategory.h
          ui/QtBookmarkCreator.cpp
          ui/QtBookmarkCreator.h
          ui/QtBookmarkView.cpp
          ui/QtBookmarkView.h)

if(ENABLE_UNIT_TEST)
  add_sourcetrail_test(
    NAME BookmarkControllerTestSuite
    SOURCES tests/BookmarkControllerTestSuite.cpp
    DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
         Sourcetrail::lib::proto::convert
    TEST_PREFIX "unittests.lib.")

  target_compile_definitions(BookmarkControllerTestSuite PUBLIC ST_TESTING=1)
endif()

if(ENABLE_INTEGRATION_TEST)
  # The Data tier's own suite: a real SQLite round-trip, hence the integration prefix rather than
  # the unit one BookmarkControllerTestSuite uses. Two tiers, two suites, one feature directory.
  add_sourcetrail_test(
    NAME SqliteBookmarkStorageTestSuite
    SOURCES tests/SqliteBookmarkStorageTestSuite.cpp
    DEPS GTest::gtest Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::file::FileSystem
    TEST_PREFIX "integration.lib.")
endif()
