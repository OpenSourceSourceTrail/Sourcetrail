# ${CMAKE_SOURCE_DIR}/cmake/copy_qt_binaries.cmake

function(copy_qt_binaries DESTINATION IS_APP)
  if(NOT EXISTS ${DESTINATION})
    file(
      MAKE_DIRECTORY
      ${DESTINATION}
      ${DESTINATION}/platforms
      ${DESTINATION}/styles
      ${DESTINATION}/imageformats)
  endif()

  get_filename_component(QT_BINARY_DIR "${QT_MOC_EXECUTABLE}" PATH)
  set(QT_PLUGINS_DIR "${QT_BINARY_DIR}/../plugins")

  set(CONFIGURATION "Release")
  set(SUFFIX "")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SUFFIX "d")
    set(CONFIGURATION "Debug")
  endif()

  set(GUI_LIBRARIES
      Qt5Core
      Qt5Gui
      Qt5Network
      Qt5Svg
      Qt5Widgets)

  if(IS_APP)
    set(SETUP_URL "https://codeload.github.com/OpenSourceSourceTrail/setup/zip/refs/heads/main")
    set(TEMP_DIR $ENV{TMP})
    file(DOWNLOAD ${SETUP_URL} ${TEMP_DIR}/setup.zip EXPECTED_HASH MD5=f9d2c22a6bba3daac09f2e1f8b237bcf)
    file(
      ARCHIVE_EXTRACT
      INPUT
      ${TEMP_DIR}/setup.zip
      DESTINATION
      ${TEMP_DIR}/)
    file(GLOB MY_PUBLIC_HEADERS "${TEMP_DIR}/setup-main/dynamic_libraries/win64/app/${CONFIGURATION}/*")
    file(COPY ${MY_PUBLIC_HEADERS} DESTINATION "${DESTINATION}")
  else()
    list(APPEND GUI_LIBRARIES Qt5Test)
  endif()

  foreach(library IN LISTS GUI_LIBRARIES)
    configure_file("${QT_BINARY_DIR}/${library}${SUFFIX}.dll" "${DESTINATION}" COPYONLY)
  endforeach()

  configure_file("${QT_PLUGINS_DIR}/platforms/qwindows${SUFFIX}.dll" "${DESTINATION}/platforms/" COPYONLY)

  configure_file("${QT_PLUGINS_DIR}/styles/qwindowsvistastyle${SUFFIX}.dll" "${DESTINATION}/styles/" COPYONLY)

  set(IMAGE_FORMATS
      qgif
      qicns
      qico
      qjpeg
      qsvg
      qtga
      qtiff
      qwbmp
      qwebp)

  foreach(image_format IN LISTS IMAGE_FORMATS)
    configure_file("${QT_PLUGINS_DIR}/imageformats/${image_format}${SUFFIX}.dll" "${DESTINATION}/imageformats/"
                   COPYONLY)
  endforeach()
endfunction()
