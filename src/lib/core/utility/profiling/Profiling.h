#pragma once

// Tracy zone macros for the indexing path, compiled away entirely unless the build was configured
// with -DENABLE_TRACY=ON.
//
// Tracy's own API is macro-based, so this shim is too. Only the handful of macros the indexing path
// actually uses are wrapped -- include <tracy/Tracy.hpp> directly rather than growing this list
// ahead of a call site.
//
// Every process that links a zone has to open the profiler itself: the client is built with
// TRACY_MANUAL_LIFETIME so that each one can pick its own listen port, because a Tracy client binds
// exactly one and the indexer workers inherit the engine's environment. profiling::Scope in main()
// is what does that; a zone reached without it is undefined behaviour, which is why all four
// executables declare one.

#if defined(SR_TRACY_ENABLED)

#  include <cstdio>
#  include <cstdlib>
#  include <string>

#  include <tracy/Tracy.hpp>

/** Zone named by a compile-time literal. */
#  define SR_ZONE_N(name) ZoneScopedN(name)
/** Zone whose name is only known at run time; pair with SR_ZONE_SET_NAME. */
#  define SR_ZONE_DYNAMIC(var, placeholder) ZoneNamedN(var, placeholder, true)
#  define SR_ZONE_SET_NAME(var, ptr, len) (var).Name((ptr), (len))
/** Free-text annotation on the innermost SR_ZONE_N, e.g. the source file being parsed. */
#  define SR_ZONE_TEXT(ptr, len) ZoneText((ptr), (len))
#  define SR_PLOT(name, value) TracyPlot(name, value)
#  define SR_FRAME_MARK(name) FrameMarkNamed(name)
#  define SR_THREAD_NAME(name) tracy::SetThreadName(name)

#else

#  define SR_ZONE_N(name) (void)0
#  define SR_ZONE_DYNAMIC(var, placeholder) (void)0
#  define SR_ZONE_SET_NAME(var, ptr, len) (void)0
#  define SR_ZONE_TEXT(ptr, len) (void)0
#  define SR_PLOT(name, value) (void)0
#  define SR_FRAME_MARK(name) (void)0
#  define SR_THREAD_NAME(name) (void)0

#endif

namespace profiling {

/**
 * Tracy's own default. The engine -- whether hosted by the GUI, standalone, or the CLI -- takes it;
 * indexer workers offset from it by their process id so the two can be captured at once.
 */
constexpr unsigned short DefaultPort = 8086;

/**
 * Opens the Tracy client on \p port for as long as it is alive. Declare one as the first thing in
 * main(); everything else is a no-op without it.
 *
 * The port is passed through TRACY_PORT rather than a compile-time define because the client is one
 * static library shared by every executable, and the engine and its workers have to listen on
 * different ports to be captured at the same time.
 */
class Scope {
public:
  explicit Scope([[maybe_unused]] unsigned short port) {
#if defined(SR_TRACY_ENABLED)
    const std::string portText = std::to_string(port);
#  ifdef _WIN32
    ::_putenv_s("TRACY_PORT", portText.c_str());
#  else
    ::setenv("TRACY_PORT", portText.c_str(), 1);
#  endif
    tracy::StartupProfiler();
    std::fprintf(stderr, "TRACY_PORT %s\n", portText.c_str());
    std::fflush(stderr);
#endif
  }

  ~Scope() {
#if defined(SR_TRACY_ENABLED)
    tracy::ShutdownProfiler();
#endif
  }

  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
  Scope(Scope&&) = delete;
  Scope& operator=(Scope&&) = delete;
};

}    // namespace profiling
