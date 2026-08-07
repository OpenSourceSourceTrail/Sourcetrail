#pragma once

#include "FilePath.h"

/**
 * Qt-free resolution of the application and user data directories.
 *
 * The GUI resolves these in lib_gui's platform_includes/includes{Windows,Linux,Mac}.h via Qt. The
 * engine and CLI are Qt-free but MUST land on the exact same directories -- a divergent user data
 * path means the processes read different ApplicationSettings.xml files and disagree about
 * everything. This is that shared, Qt-free implementation.
 */
namespace platform_paths {

/** Directory containing the running executable, falling back to the current working directory. */
FilePath getExecutableDirectory();

/**
 * Sets AppPath's shared data / cxx indexer directories and UserPaths' user data directory,
 * matching the layout the GUI produces on the same platform. Creates the user data directory.
 */
void setupPaths();

}    // namespace platform_paths
