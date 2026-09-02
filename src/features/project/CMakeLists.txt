# ${PROJECT_SOURCE_DIR}/src/features/project/CMakeLists.txt
#
# The project feature: source groups, the project state machine, and the wizard that edits them.
# See src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# This is the feature where the tier split is load-bearing rather than cosmetic:
#
#   logic/  SourceGroup* and IProject -- describing what to index. No SQLite.
#   data/   Project, IndexTaskBuilder, RefreshInfoGenerator, the task factories -- these read and
#           write the index, so they are Sourcetrail_lib_engine and unreachable from ui/.
#
# ui/ keeps the wizard's own subdirectory tree (content/, content/paths/, ...). It is 61 files; a
# flat directory would be worse than the nesting.
#
# Moving MessageLoadProject here removes the last header that forced Sourcetrail_messaging to link
# Sourcetrail_lib -- it carried project/RefreshInfo.h by value. That PUBLIC link and its TODO are
# now gone from src/lib/messaging/CMakeLists.txt.

target_sources(
  Sourcetrail_lib
  PRIVATE domain/RefreshInfo.h
          logic/CompilationDatabase.cpp
          logic/CompilationDatabase.h
          logic/CxxToolchainRemote.cpp
          logic/CxxToolchainRemote.h
          logic/ICxxToolchain.cpp
          logic/ICxxToolchain.h
          logic/IProject.cpp
          logic/IProject.hpp
          logic/SourceGroup.cpp
          logic/SourceGroup.h
          logic/SourceGroupCustomCommand.cpp
          logic/SourceGroupCustomCommand.h
          logic/SourceGroupCxxCdb.cpp
          logic/SourceGroupCxxCdb.h
          logic/SourceGroupCxxEmpty.cpp
          logic/SourceGroupCxxEmpty.h
          logic/SourceGroupFactory.cpp
          logic/SourceGroupFactory.h
          logic/SourceGroupFactoryModule.cpp
          logic/SourceGroupFactoryModule.h
          logic/SourceGroupFactoryModuleCustom.cpp
          logic/SourceGroupFactoryModuleCustom.h
          logic/SourceGroupFactoryModuleCxx.cpp
          logic/SourceGroupFactoryModuleCxx.h
          logic/SourceGroupFactoryModuleJava.cpp
          logic/SourceGroupFactoryModuleJava.h
          logic/SourceGroupJavaEmpty.cpp
          logic/SourceGroupJavaEmpty.h
          logic/utilitySourceGroupCxx.cpp
          logic/utilitySourceGroupCxx.h
          messages/MessageCloseProject.h
          messages/MessageLoadProject.cpp
          messages/MessageLoadProject.h
          messages/MessageProjectEdit.h
          messages/MessageProjectNew.h)

target_sources(
  Sourcetrail_lib_engine
  PRIVATE data/DefaultTaskFactory.cpp
          data/DefaultTaskFactory.h
          data/ITaskFactory.h
          data/IndexTaskBuilder.cpp
          data/IndexTaskBuilder.h
          data/Project.cpp
          data/Project.h
          data/RefreshInfoGenerator.cpp
          data/RefreshInfoGenerator.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtProjectWizard.cpp
          ui/QtProjectWizard.h
          ui/QtProjectWizardWindow.cpp
          ui/QtProjectWizardWindow.h
          ui/content/QtProjectWizardContent.cpp
          ui/content/QtProjectWizardContent.h
          ui/content/QtProjectWizardContentCStandard.cpp
          ui/content/QtProjectWizardContentCStandard.h
          ui/content/QtProjectWizardContentCppStandard.cpp
          ui/content/QtProjectWizardContentCppStandard.h
          ui/content/QtProjectWizardContentCrossCompilationOptions.cpp
          ui/content/QtProjectWizardContentCrossCompilationOptions.h
          ui/content/QtProjectWizardContentCustomCommand.cpp
          ui/content/QtProjectWizardContentCustomCommand.h
          ui/content/QtProjectWizardContentCxxPchFlags.cpp
          ui/content/QtProjectWizardContentCxxPchFlags.h
          ui/content/QtProjectWizardContentExtensions.cpp
          ui/content/QtProjectWizardContentExtensions.h
          ui/content/QtProjectWizardContentFlags.cpp
          ui/content/QtProjectWizardContentFlags.h
          ui/content/QtProjectWizardContentGroup.cpp
          ui/content/QtProjectWizardContentGroup.h
          ui/content/QtProjectWizardContentPreferences.cpp
          ui/content/QtProjectWizardContentPreferences.h
          ui/content/QtProjectWizardContentProjectData.cpp
          ui/content/QtProjectWizardContentProjectData.h
          ui/content/QtProjectWizardContentRequiredLabel.h
          ui/content/QtProjectWizardContentSelect.cpp
          ui/content/QtProjectWizardContentSelect.h
          ui/content/QtProjectWizardContentSourceGroupData.cpp
          ui/content/QtProjectWizardContentSourceGroupData.h
          ui/content/QtProjectWizardContentSourceGroupInfoText.cpp
          ui/content/QtProjectWizardContentSourceGroupInfoText.h
          ui/content/QtProjectWizardContentUnloadable.cpp
          ui/content/QtProjectWizardContentUnloadable.h
          ui/content/QtProjectWizardContentVS.cpp
          ui/content/QtProjectWizardContentVS.h
          ui/content/path/ProjectWizardModel.cpp
          ui/content/path/ProjectWizardModel.hpp
          ui/content/path/QtProjectWizardContentPath.cpp
          ui/content/path/QtProjectWizardContentPath.h
          ui/content/path/QtProjectWizardContentPathCDB.cpp
          ui/content/path/QtProjectWizardContentPathCDB.h
          ui/content/path/QtProjectWizardContentPathCxxPch.cpp
          ui/content/path/QtProjectWizardContentPathCxxPch.h
          ui/content/paths/QtProjectWizardContentPaths.cpp
          ui/content/paths/QtProjectWizardContentPaths.h
          ui/content/paths/QtProjectWizardContentPathsExclude.cpp
          ui/content/paths/QtProjectWizardContentPathsExclude.h
          ui/content/paths/QtProjectWizardContentPathsFrameworkSearch.cpp
          ui/content/paths/QtProjectWizardContentPathsFrameworkSearch.h
          ui/content/paths/QtProjectWizardContentPathsFrameworkSearchGlobal.cpp
          ui/content/paths/QtProjectWizardContentPathsFrameworkSearchGlobal.h
          ui/content/paths/QtProjectWizardContentPathsHeaderSearch.cpp
          ui/content/paths/QtProjectWizardContentPathsHeaderSearch.h
          ui/content/paths/QtProjectWizardContentPathsHeaderSearchGlobal.cpp
          ui/content/paths/QtProjectWizardContentPathsHeaderSearchGlobal.h
          ui/content/paths/QtProjectWizardContentPathsIndexedHeaders.cpp
          ui/content/paths/QtProjectWizardContentPathsIndexedHeaders.h
          ui/content/paths/QtProjectWizardContentPathsSource.cpp
          ui/content/paths/QtProjectWizardContentPathsSource.h)

if(ENABLE_UNIT_TEST)
  foreach(test_name IN ITEMS ProjectTestSuite utilitySourceGroupCxxTestSuite)
    add_sourcetrail_test(
      NAME ${test_name}
      SOURCES tests/${test_name}.cpp
      DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess
           Sourcetrail::lib::proto::convert
      TEST_PREFIX "unittests.lib.")

    target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)
  endforeach()
endif()

if(ENABLE_INTEGRATION_TEST)
  # Real files and a real index, hence the integration prefix.
  foreach(test_name IN ITEMS RefreshInfoGeneratorTestSuite SourceGroupTestSuite)
    add_sourcetrail_test(
      NAME ${test_name}
      SOURCES tests/${test_name}.cpp
      DEPS GTest::gtest
           GTest::gmock
           Qt6::Core
           Qt6::Gui
           Qt6::Sql
           Sourcetrail::lib::messaging
           Sourcetrail::lib::core::utility::TextAccess
           Sourcetrail::lib::core::utility::file::FileSystem
           Sourcetrail::lib::core::utility::file::utilityFile
           $<$<BOOL:${BUILD_CXX_LANGUAGE_PACKAGE}>:Sourcetrail::lib::lib_cxx>
           lib::mocks
           Sourcetrail::lib::lib_engine
           Boost::date_time
           spdlog::spdlog
      TEST_PREFIX "integration.lib.")
  endforeach()
endif()
