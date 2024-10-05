# ${CMAKE_SOURCE_DIR}/src/messaging/CMakeLists.txt
add_library(Sourcetrail_messaging)
add_library(Sourcetrail::messaging ALIAS Sourcetrail_messaging)

target_sources(
  Sourcetrail_messaging
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/MessageBase.cpp
          ${CMAKE_CURRENT_LIST_DIR}/MessageListenerBase.cpp
          ${CMAKE_CURRENT_LIST_DIR}/MessageQueue.cpp
          ${CMAKE_CURRENT_LIST_DIR}/type/MessageLoadProject.cpp
          ${CMAKE_CURRENT_LIST_DIR}/type/MessageStatus.cpp)

target_include_directories(Sourcetrail_messaging PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(
  Sourcetrail_messaging
  PUBLIC Sourcetrail::lib # TODO(Hussein): Needed for `type.h` It should be removed to Sourcetrail::utility
         Sourcetrail::lib_utility
  PRIVATE Sourcetrail::core Sourcetrail::scheduling)

if(ENABLE_UNIT_TEST)
  add_subdirectory(tests)
endif()
