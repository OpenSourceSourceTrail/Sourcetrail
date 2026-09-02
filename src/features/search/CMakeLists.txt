# ${PROJECT_SOURCE_DIR}/src/features/search/CMakeLists.txt
#
# The search feature: the search bar, its autocompletion, and the in-view screen search. See
# src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# domain/ holds both indexes as well as SearchMatch. SearchIndex and FullTextSearchIndex are
# in-memory search structures over names and file text -- they are the feature's own data
# structures, not SQLite, so they stay in Sourcetrail_lib alongside the type they produce. Note
# SearchMatch is named far outside this feature (StorageAccess, the graph and code views, the
# activation controller); that breadth is what the domain tier is for.
#
# SearchController and ScreenSearchController are one feature: both answer the same search bar, and
# MessageFind switches between them.
#
# tests/QtScreenSearchBoxTestSuite.cpp is not registered. It was already commented out of
# lib_gui/tests/gui/CMakeLists.txt before this migration; it moves with its feature rather than
# being left behind, but reviving it is separate work.

target_sources(
  Sourcetrail_lib
  PRIVATE domain/FullTextSearchIndex.cpp
          domain/FullTextSearchIndex.h
          domain/SearchIndex.cpp
          domain/SearchIndex.h
          domain/SearchMatch.cpp
          domain/SearchMatch.h
          domain/SuffixArray.cpp
          domain/SuffixArray.h
          logic/ScreenSearchController.cpp
          logic/ScreenSearchController.h
          logic/ScreenSearchInterfaces.h
          logic/ScreenSearchView.cpp
          logic/ScreenSearchView.h
          logic/SearchController.cpp
          logic/SearchController.h
          logic/SearchView.cpp
          logic/SearchView.h
          messages/MessageFilterSearchAutocomplete.h
          messages/MessageFind.h
          messages/MessageSearch.h
          messages/MessageSearchAutocomplete.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtAutocompletionList.cpp
          ui/QtAutocompletionList.h
          ui/QtScreenSearchBox.cpp
          ui/QtScreenSearchBox.h
          ui/QtScreenSearchView.cpp
          ui/QtScreenSearchView.h
          ui/QtSearchBar.cpp
          ui/QtSearchBar.h
          ui/QtSearchBarButton.cpp
          ui/QtSearchBarButton.h
          ui/QtSearchView.cpp
          ui/QtSearchView.h
          ui/QtSmartSearchBox.cpp
          ui/QtSmartSearchBox.h)

if(ENABLE_UNIT_TEST)
  foreach(test_name IN ITEMS ScreenSearchControllerTestSuite SearchControllerTestSuite SearchIndexTestSuite)
    add_sourcetrail_test(
      NAME ${test_name}
      SOURCES tests/${test_name}.cpp
      DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
           Sourcetrail::lib::proto::convert
      TEST_PREFIX "unittests.lib.")

    target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)
  endforeach()
endif()
