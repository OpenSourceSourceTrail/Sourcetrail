# Tier boundary enforcement for the three-tier architecture.
#
# Presentation (Sourcetrail_lib_gui) may depend on Logic (Sourcetrail_lib); it must never reach
# Data (Sourcetrail_lib_engine) or SQLite. The GUI reads the index through StorageAccess, whose
# implementation is chosen at the composition root -- in-process or over HTTP.
#
# This asserts on the *library* target, not the Sourcetrail *binary*: that binary hosts the engine
# in-process by default and so legitimately contains SQLite. The rule is about which library may
# name which, and only a target-level check can express it. It runs at configure time, needs no
# nm, and works on every platform.

# Collects TARGET's transitive link closure into OUT_VAR, recording for each visited target the
# path that reached it, so a violation can be reported as a chain rather than a bare name.
function(_st_collect_link_closure TARGET OUT_VAR)
  set(_pending "${TARGET}")
  set(_seen "")
  set(_closure "")

  while(_pending)
    list(POP_FRONT _pending _current)
    string(REGEX REPLACE " *-> *" ";" _chain "${_current}")
    list(GET _chain -1 _node)

    if(_node IN_LIST _seen)
      continue()
    endif()
    list(APPEND _seen "${_node}")
    list(APPEND _closure "${_current}")

    if(NOT TARGET ${_node})
      continue()
    endif()

    # An INTERFACE library has no LINK_LIBRARIES, only INTERFACE_LINK_LIBRARIES; reading the
    # former on one is an error, so ask for the type first.
    get_target_property(_type ${_node} TYPE)
    set(_deps "")
    if(NOT _type STREQUAL "INTERFACE_LIBRARY")
      get_target_property(_link ${_node} LINK_LIBRARIES)
      if(_link)
        list(APPEND _deps ${_link})
      endif()
    endif()
    get_target_property(_iface ${_node} INTERFACE_LINK_LIBRARIES)
    if(_iface)
      list(APPEND _deps ${_iface})
    endif()

    foreach(_dep IN LISTS _deps)
      # Skip generator expressions: they are not resolved at configure time. Link-only
      # $<LINK_ONLY:...> wrappers come from PRIVATE links and are unwrapped rather than dropped.
      if(_dep MATCHES "^\\$<LINK_ONLY:(.+)>$")
        set(_dep "${CMAKE_MATCH_1}")
      elseif(_dep MATCHES "\\$<")
        continue()
      endif()
      list(APPEND _pending "${_current} -> ${_dep}")
    endforeach()
  endwhile()

  set(${OUT_VAR}
      "${_closure}"
      PARENT_SCOPE)
endfunction()

# assert_no_transitive_link(<target> <forbidden>...)
#
# Fails configuration if <target> can reach any <forbidden> target through its link graph.
function(assert_no_transitive_link TARGET)
  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR "assert_no_transitive_link: no such target '${TARGET}'")
  endif()

  _st_collect_link_closure(${TARGET} _closure)

  set(_violations "")
  foreach(_path IN LISTS _closure)
    string(REGEX REPLACE " *-> *" ";" _chain "${_path}")
    list(GET _chain -1 _node)
    if(_node IN_LIST ARGN)
      list(APPEND _violations "    ${_path}")
    endif()
  endforeach()

  if(_violations)
    string(REPLACE ";" "\n" _report "${_violations}")
    message(
      FATAL_ERROR
        "Tier violation: ${TARGET} must not depend on ${ARGN}.\n"
        "The Presentation tier reads the index through StorageAccess, never through storage " "types directly.\n"
        "Offending link path(s):\n${_report}\n")
  endif()

  message(STATUS "Tier guard: ${TARGET} is free of [${ARGN}].")
endfunction()
