# ${CMAKE_SOURCE_DIR}/cmake/create_symlink.cmake

function(create_symlink SOURCE DESTINATION)
  if(WIN32)
    string(
      REGEX
      REPLACE "/"
              "\\\\"
              BACKSLASHED_SOURCE_DIR
              ${SOURCE})
    string(
      REGEX
      REPLACE "/"
              "\\\\"
              BACKSLASHED_DESTINATION_DIR
              ${DESTINATION})
    execute_process(COMMAND "cmd.exe" "/k" "rmdir" "${BACKSLASHED_DESTINATION_DIR}" & "mklink" "/d" "/j"
                            "${BACKSLASHED_DESTINATION_DIR}" "${BACKSLASHED_SOURCE_DIR}" & exit)
  else()
    file(
      CREATE_LINK
      "${SOURCE}"
      "${DESTINATION}"
      SYMBOLIC)
  endif()
endfunction()
