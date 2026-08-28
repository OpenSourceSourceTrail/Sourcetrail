# The project's warning set, as a single INTERFACE target.
#
# Flag lists originally from
# https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md
#
# Targets link this PRIVATE so the flags apply to their own sources and do not propagate to
# consumers. The previous per-target function set these as INTERFACE compile options on static
# libraries, which meant the opposite: a library's own translation units were compiled unwarned and
# the flags leaked to whoever linked it. Most sources still got warned by accident, through the
# INTERFACE libraries they happen to link; the nine leaf utility targets that link none did not.

set(SOURCETRAIL_MSVC_WARNINGS
    /W4 # Baseline reasonable warnings
    /w14242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
    /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
    /w14263 # 'function': member function does not override any base class virtual member function
    /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be
            # destructed correctly
    /w14287 # 'operator': unsigned/negative constant mismatch
    /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the
            # for-loop scope
    /w14296 # 'operator': expression is always 'boolean_value'
    /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
    /w14545 # expression before comma evaluates to a function which is missing an argument list
    /w14546 # function call before comma missing argument list
    /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
    /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
    /w14555 # expression has no effect; expected expression with side- effect
    /w14619 # pragma warning: there is no warning number 'number'
    /w14640 # Enable warning on thread un-safe static member initialization
    /w14826 # Conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior.
    /w14905 # wide string literal cast to 'LPSTR'
    /w14906 # string literal cast to 'LPWSTR'
    /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
    /permissive- # standards conformance mode for MSVC compiler.
    /Zc:preprocessor # to use __VA_OPT__ in macros
)

set(SOURCETRAIL_COMMON_WARNINGS
    -Wall
    -Wextra # reasonable and standard
    -Wpedantic # warn if non-standard C++ is used
    -Wshadow # warn the user if a variable declaration shadows one from a parent context
    -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch
                       # hard to track down memory errors
    -Wold-style-cast # warn for c-style casts
    -Wcast-align # warn for potential performance problem casts
    -Wunused # warn on anything being unused
    -Wno-overloaded-virtual
    -Wconversion # warn on type conversions that may lose data
    -Wsign-conversion # warn on sign conversions
    -Wnull-dereference # warn if a null dereference is detected
    -Wdouble-promotion # warn if float is implicit promoted to double
    -Wformat=2 # warn on security issues around functions that format output (ie printf)
    -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
)

set(SOURCETRAIL_CLANG_WARNINGS
    ${SOURCETRAIL_COMMON_WARNINGS}
    -Wc++20-compat-pedantic
    -Wcall-to-pure-virtual-from-ctor-dtor
    -Wcalled-once-parameter
    -Wcast-calling-convention
    -Wcast-function-type
    -Wcast-function-type-strict
    -Wcast-of-sel-type
    -Wcast-qual
    -Wcast-qual-unrelated
    -Wdangling
    -Wint-to-pointer-cast
    -Wunreachable-code
    -Wuninitialized
    -Wthread-safety
    -Wswitch)

set(SOURCETRAIL_GCC_WARNINGS
    ${SOURCETRAIL_COMMON_WARNINGS}
    -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
    -Wduplicated-cond # warn if if / else chain has duplicated conditions
    -Wduplicated-branches # warn if if / else branches have duplicated code
    -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
    -Wuseless-cast # warn if you perform a cast to the same type
    -Wundef
    -Wformat-truncation)

if(MSVC)
  set(SOURCETRAIL_WARNINGS ${SOURCETRAIL_MSVC_WARNINGS})
  set(_sourcetrail_werror /WX)
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(SOURCETRAIL_WARNINGS ${SOURCETRAIL_CLANG_WARNINGS})
  set(_sourcetrail_werror -Werror)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(SOURCETRAIL_WARNINGS ${SOURCETRAIL_GCC_WARNINGS})
  set(_sourcetrail_werror -Werror)
else()
  message(AUTHOR_WARNING "No compiler warnings set for CXX compiler: '${CMAKE_CXX_COMPILER_ID}'")
  set(SOURCETRAIL_WARNINGS "")
  set(_sourcetrail_werror "")
endif()

if(SOURCETRAIL_WARNING_AS_ERROR)
  list(APPEND SOURCETRAIL_WARNINGS ${_sourcetrail_werror})
endif()

add_library(Sourcetrail_warnings INTERFACE)
add_library(Sourcetrail::warnings ALIAS Sourcetrail_warnings)

# The same flags for C and C++; the project enables no other language.
target_compile_options(Sourcetrail_warnings INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${SOURCETRAIL_WARNINGS}>
                                                      $<$<COMPILE_LANGUAGE:C>:${SOURCETRAIL_WARNINGS}>)

target_compile_definitions(Sourcetrail_warnings INTERFACE $<$<CONFIG:Debug>:ST_DEBUG>)
