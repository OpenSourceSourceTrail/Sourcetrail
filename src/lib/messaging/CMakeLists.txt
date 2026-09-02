# ${PROJECT_SOURCE_DIR}/src/messaging/CMakeLists.txt
add_library(Sourcetrail_messaging)
add_library(Sourcetrail::lib::messaging ALIAS Sourcetrail_messaging)

target_sources(
  Sourcetrail_messaging
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/MessageBase.cpp ${CMAKE_CURRENT_LIST_DIR}/MessageListenerBase.cpp
          ${CMAKE_CURRENT_LIST_DIR}/MessageQueue.cpp ${CMAKE_CURRENT_LIST_DIR}/TabId.cpp
          ${CMAKE_CURRENT_LIST_DIR}/TabId.h)

target_include_directories(Sourcetrail_messaging PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(
  Sourcetrail_messaging
  # No link to Sourcetrail::lib::lib. The bus is now the bus: Message, MessageBase,
  # MessageListenerBase, MessageQueue and TabId. Every message that carries a Logic type by value
  # lives in the feature that owns that type.
  PUBLIC Sourcetrail::lib::core::utility::math Sourcetrail::lib::core::utility::GlobalId
  PRIVATE Sourcetrail::lib::core Sourcetrail::lib::scheduling Sourcetrail::lib::core::utility::logging)

if(ENABLE_UNIT_TEST)
  add_subdirectory(tests)
endif()

# Project warning set. PRIVATE so it applies here and is not inflicted on consumers.
target_link_libraries(Sourcetrail_messaging PRIVATE Sourcetrail::warnings)
