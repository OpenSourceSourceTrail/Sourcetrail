# ${CMAKE_SOURCE_DIR}/src/scheduling/CMakeLists.txt
add_library(Sourcetrail_scheduling)
add_library(Sourcetrail::scheduling ALIAS Sourcetrail_scheduling)

target_sources(
  Sourcetrail_scheduling
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Blackboard.cpp
          ${CMAKE_CURRENT_LIST_DIR}/ITaskManager.cpp
          ${CMAKE_CURRENT_LIST_DIR}/Task.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskScheduler.cpp
          ${CMAKE_CURRENT_LIST_DIR}/impls/TaskManager.cpp)

if(SOURCETRAIL_WASM)
  target_sources(
    Sourcetrail_scheduling
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/TaskDecorator.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskDecoratorDelay.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskDecoratorRepeat.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskFindKeyOnBlackboard.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskGroup.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskGroupParallel.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskGroupSelector.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskGroupSequence.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskLambda.cpp
            ${CMAKE_CURRENT_LIST_DIR}/TaskRunner.cpp)
endif()

target_include_directories(Sourcetrail_scheduling PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(
  Sourcetrail_scheduling
  PUBLIC Sourcetrail::core Sourcetrail::core::utility::logging Sourcetrail::core::utility::GlobalId
  PRIVATE Boost::date_time Sourcetrail::core::utility::ScopedFunctor)

if(ENABLE_UNIT_TEST)
  add_subdirectory(mocks)
  add_subdirectory(tests)
endif()
