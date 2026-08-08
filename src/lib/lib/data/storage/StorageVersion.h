#pragma once
#include <cstddef>

/**
 * Schema version of the index database.
 *
 * It lives here rather than on SqliteIndexStorage because three GUI labels and the custom-command
 * substitution only want the number, and pulling in the SQLite storage header for a constant is
 * what kept the GUI linked against SQLite.
 */
constexpr size_t kStorageVersion = 25;
