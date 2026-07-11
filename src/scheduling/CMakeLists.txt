# ${CMAKE_SOURCE_DIR}/src/scheduling/CMakeLists.txt
add_library(Sourcetrail_scheduling)
add_library(Sourcetrail::scheduling ALIAS Sourcetrail_scheduling)

target_sources(
  Sourcetrail_scheduling
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Blackboard.cpp
          ${CMAKE_CURRENT_LIST_DIR}/Task.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskDecorator.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskDecoratorDelay.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskDecoratorRepeat.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskDispatchQueue.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskDispatchRegistry.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskFindKeyOnBlackboard.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskGroup.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskGroupSelector.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskGroupSequence.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskflowGroupParallel.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskLambda.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TaskRunner.cpp)

target_include_directories(Sourcetrail_scheduling PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(
  Sourcetrail_scheduling
  PUBLIC Sourcetrail::core Sourcetrail::core::utility::logging Sourcetrail::core::utility::GlobalId
         Sourcetrail::core::utility::DispatchQueue
  PRIVATE Boost::date_time Sourcetrail::core::utility::ScopedFunctor Sourcetrail::core::utility::TaskExecutor)
