# ${PROJECT_SOURCE_DIR}/src/features/indexing/CMakeLists.txt
#
# The indexing feature: describing the work, farming it out to worker processes, and the dialogs
# that report on it. See src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# This is the first feature with a real data/ tier. TaskExecuteCustomCommands runs in
# Sourcetrail_lib_engine because it writes into the index; everything else here only *describes*
# work (IndexerCommand and its providers) or schedules it (TaskBuildIndex, the gRPC client and the
# worker service), which is Logic and stays out of lib_engine.
#
# logic/grpc/ keeps its subdirectory: it is the engine<->worker transport, a distinct enough
# concern to stay named, and flattening it would put GrpcIndexer next to the Indexer interface it
# implements over the wire.

target_sources(
  Sourcetrail_lib
  PRIVATE domain/IndexerCommandType.cpp
          domain/IndexerCommandType.h
          domain/IndexerStateInfo.h
          domain/IndexingPhaseStats.cpp
          domain/IndexingPhaseStats.h
          logic/CombinedIndexerCommandProvider.cpp
          logic/CombinedIndexerCommandProvider.h
          logic/CxxIndexerCommandProvider.cpp
          logic/CxxIndexerCommandProvider.h
          logic/Indexer.h
          logic/IndexerBase.cpp
          logic/IndexerBase.h
          logic/IndexerCommand.cpp
          logic/IndexerCommand.h
          logic/IndexerCommandCustom.cpp
          logic/IndexerCommandCustom.h
          logic/IndexerCommandCxx.cpp
          logic/IndexerCommandCxx.h
          logic/IndexerCommandJava.cpp
          logic/IndexerCommandJava.h
          logic/IndexerCommandProvider.cpp
          logic/IndexerCommandProvider.h
          logic/IndexerComposite.cpp
          logic/IndexerComposite.h
          logic/MemoryIndexerCommandProvider.cpp
          logic/MemoryIndexerCommandProvider.h
          logic/TaskBuildIndex.cpp
          logic/TaskBuildIndex.h
          logic/TaskFillIndexerCommandQueue.cpp
          logic/TaskFillIndexerCommandQueue.h
          logic/grpc/GrpcIndexer.cpp
          logic/grpc/GrpcIndexer.h
          logic/grpc/IndexerWorkerServiceImpl.cpp
          logic/grpc/IndexerWorkerServiceImpl.h
          messages/MessageIndexingFinished.h
          messages/MessageIndexingInterrupted.h
          messages/MessageIndexingShowDialog.h
          messages/MessageIndexingStarted.h
          messages/MessageIndexingStatus.h)

target_sources(Sourcetrail_lib_engine PRIVATE data/TaskExecuteCustomCommands.cpp data/TaskExecuteCustomCommands.h)

target_sources(
  Sourcetrail_lib_gui
  PRIVATE ui/QtIndexingDialog.cpp
          ui/QtIndexingDialog.h
          ui/QtIndexingProgressDialog.cpp
          ui/QtIndexingProgressDialog.h
          ui/QtIndexingReportDialog.cpp
          ui/QtIndexingReportDialog.h
          ui/QtIndexingStartDialog.cpp
          ui/QtIndexingStartDialog.h)

if(ENABLE_UNIT_TEST)
  foreach(test_name IN ITEMS IndexerCommandJavaTestSuite IndexerCompositeTestSuite TaskBuildIndexTestSuite)
    add_sourcetrail_test(
      NAME ${test_name}
      SOURCES tests/${test_name}.cpp
      DEPS lib_test_utilities lib::mocks Sourcetrail::lib::lib_engine Sourcetrail::lib::core::utility::TextAccess Qt6::Gui
           Sourcetrail::lib::proto::convert
      TEST_PREFIX "unittests.lib.")

    target_compile_definitions(${test_name} PUBLIC ST_TESTING=1)
  endforeach()
endif()
