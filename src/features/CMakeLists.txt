# ${PROJECT_SOURCE_DIR}/src/features/CMakeLists.txt
#
# Feature directories. Each one contributes sources to the three tier targets rather than declaring
# targets of its own -- see src/features/bookmark/CMakeLists.txt for the shape and the reasoning.
#
# This directory is the include root for all of them, so an include reads
# "<feature>/<tier>/<File>.h". Added to each tier target here, once, rather than in every feature.
foreach(tier_target IN ITEMS Sourcetrail_lib Sourcetrail_lib_engine Sourcetrail_lib_gui)
  target_include_directories(${tier_target} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
endforeach()

add_subdirectory(bookmark)
add_subdirectory(history)
add_subdirectory(tooltip)
