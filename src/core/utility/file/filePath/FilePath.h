#pragma once
/**
 * @file FilePath.h
 * @brief A robust file path management class using standard filesystem
 *
 * @details This class provides comprehensive file path manipulation and query capabilities,
 * offering a high-level abstraction over std::filesystem::path with additional
 * functionality for path operations, validation, and environment variable expansion.
 *
 * @note The class is thread-safe for const methods but not for mutation operations
 */
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/**
 * @class FilePath
 * @brief A robust, cross-platform file path management class
 *
 * @details
 * The FilePath class provides comprehensive file path manipulation,
 * validation, and transformation capabilities. It serves as a high-level
 * wrapper around std::filesystem::path with additional features for
 * path handling, environment variable expansion, and cross-platform
 * filesystem operations.
 *
 * Key Features:
 * - Path validation and existence checking
 * - Absolute and relative path transformations
 * - Extension and filename manipulation
 * - Environment variable expansion
 * - Robust path comparison and containment checks
 *
 * @note
 * - Supports both narrow and wide string path representations
 * - Provides caching mechanisms for performance optimization
 * - Implements RAII (Resource Acquisition Is Initialization) principle
 *
 * @warning
 * - Not thread-safe for mutation operations
 * - Relies on std::filesystem for core path manipulations
 *
 * @example
 * ```cpp
 * // Creating and manipulating file paths
 * FilePath path("~/documents/file.txt");
 * FilePath absolutePath = path.getAbsolute();
 * std::wstring filename = path.fileName();
 * bool exists = path.exists();
 * ```
 *
 * @see std::filesystem
 */
class FilePath final {
public:
  /**
   * @brief Default constructor creating an empty file path
   *
   * Initializes an empty, invalid file path
   */
  FilePath();

  /**
   * @brief Construct a FilePath from a standard string
   *
   * @param filePath String representation of the file path
   * @throws Potential exceptions from underlying filesystem operations
   */
  explicit FilePath(const std::string& filePath);

  /**
   * @brief Construct a FilePath from a wide string (UTF-16/UTF-32)
   *
   * @param filePath Wide string representation of the file path
   * @throws Potential exceptions from underlying filesystem operations
   */
  explicit FilePath(const std::wstring& filePath);

  /**
   * @brief Copy constructor
   *
   * @param other Source FilePath to copy
   */
  FilePath(const FilePath& other);

  /**
   * @brief Move constructor
   *
   * @param other Source FilePath to move, leaving it in a valid but unspecified state
   * @note Noexcept guarantee provided
   */
  FilePath(FilePath&& other) noexcept;

  /**
   * @brief Construct a FilePath with a base directory
   *
   * @param filePath Path to be resolved
   * @param base Base directory for resolving relative paths
   */
  FilePath(const std::wstring& filePath, const std::wstring& basePath);

  /**
   * @brief Destructor
   *
   * Releases underlying filesystem resources
   */
  ~FilePath();

  /**
   * @brief Provides a singleton empty file path
   *
   * @return const reference to a static empty FilePath instance
   */
  static const FilePath& EmptyFilePath();

  // Conversion and Information Methods
  /**
   * @brief Retrieve underlying standard filesystem path
   *
   * @return std::filesystem::path representing the current path
   */
  [[nodiscard]] std::filesystem::path getPath() const;

  /**
   * @brief Check if the path is empty
   *
   * @return true if path is empty, false otherwise
   */
  [[nodiscard]] bool empty() const;

  /**
   * @brief Check if the file/directory exists
   *
   * @return true if path exists, false otherwise
   * @note Caches existence check for performance
   */
  [[nodiscard]] bool exists() const noexcept;

  /**
   * @brief Force recheck of file/directory existence
   *
   * @return true if path exists, false otherwise
   * @note Bypasses cached existence check
   */
  [[nodiscard]] bool recheckExists() const;
  /**
   * @brief Determine if path represents a directory
   *
   * @return true if path is a directory, false otherwise
   */
  [[nodiscard]] bool isDirectory() const noexcept;

  /**
   * @brief Check if path is absolute
   *
   * @return true for absolute paths, false for relative paths
   */
  [[nodiscard]] bool isAbsolute() const;

  /**
   * @brief Validate the path
   *
   * @return true if path is valid and well-formed, false otherwise
   */
  [[nodiscard]] bool isValid() const;

  // Path Transformation Methods
  /**
   * @brief Get parent directory of current path
   *
   * @return FilePath representing the parent directory
   */
  [[nodiscard]] FilePath getParentDirectory() const;

  /**
   * @brief Convert path to absolute
   *
   * @return Reference to current FilePath (for method chaining)
   */
  FilePath& makeAbsolute();

  /**
   * @brief Get absolute path representation
   *
   * @return New FilePath with absolute path
   */
  [[nodiscard]] FilePath getAbsolute() const;

  /**
   * @brief Canonicalize the path (resolve symlinks, normalize)
   *
   * @return Reference to current FilePath
   */
  FilePath& makeCanonical();

  /**
   * @brief Get canonicalized path representation
   *
   * @return New FilePath with canonicalized path
   */
  [[nodiscard]] FilePath getCanonical() const;

  /**
   * @brief Make path relative to another path
   *
   * @param other Base path to make current path relative to
   * @return Reference to current FilePath (for method chaining)
   * @throws Potential filesystem-related exceptions
   */
  FilePath& makeRelativeTo(const FilePath& other);

  /**
   * @brief Get relative path representation
   *
   * @param other Base path to derive relative path from
   * @return New FilePath representing the relative path
   * @throws Potential filesystem-related exceptions
   */
  [[nodiscard]] FilePath getRelativeTo(const FilePath& other) const;

  /**
   * @brief Concatenate another path to current path
   *
   * @param other FilePath to concatenate
   * @return Reference to current FilePath (for method chaining)
   */
  FilePath& concatenate(const FilePath& other);

  /**
   * @brief Get concatenated path representation
   *
   * @param other FilePath to concatenate
   * @return New FilePath with concatenated paths
   */
  [[nodiscard]] FilePath getConcatenated(const FilePath& other) const;

  /**
   * @brief Concatenate a wide string path to current path
   *
   * @param other Wide string path to concatenate
   * @return Reference to current FilePath (for method chaining)
   */
  FilePath& concatenate(const std::wstring& other);

  /**
   * @brief Get concatenated path with wide string
   *
   * @param other Wide string path to concatenate
   * @return New FilePath with concatenated paths
   */
  [[nodiscard]] FilePath getConcatenated(const std::wstring& other) const;

  /**
   * @brief Convert path to lowercase
   *
   * @return New FilePath with lowercase representation
   * @note Useful for case-insensitive path comparisons
   */
  [[nodiscard]] FilePath getLowerCase() const;

  /**
   * @brief Expand environment variables in the path
   *
   * @return Vector of FilePaths with expanded environment variables
   * @details Handles multiple potential expansions if environment variables
   * can result in different paths
   */
  [[nodiscard]] std::vector<FilePath> expandEnvironmentVariables() const;

  /**
   * @brief Expand environment variables using standard filesystem
   *
   * @return Vector of standard filesystem paths
   * @note Alternative method using std::filesystem
   */
  [[nodiscard]] std::vector<std::filesystem::path> expandEnvironmentVariablesStl() const;

  /**
   * @brief Check if current path contains another path
   *
   * @param other Path to check for containment
   * @return true if current path contains the other path, false otherwise
   * @note Performs a logical containment check, not just string comparison
   */
  [[nodiscard]] bool contains(const FilePath& other) const;

  /**
   * @brief Get path as narrow string
   *
   * @return Standard string representation of the path
   */
  [[nodiscard]] std::string str() const;

  /**
   * @brief Get path as wide string
   *
   * @return Wide string representation of the path
   */
  [[nodiscard]] std::wstring wstr() const;

  /**
   * @brief Extract filename from the path
   *
   * @return Wide string containing just the filename
   */
  [[nodiscard]] std::wstring fileName() const;

  /**
   * @brief Get file extension
   *
   * @return Wide string of the file extension (including the dot)
   */
  [[nodiscard]] std::wstring extension() const;

  /**
   * @brief Remove file extension
   *
   * @return New FilePath without the extension
   */
  [[nodiscard]] FilePath withoutExtension() const;

  /**
   * @brief Replace file extension
   *
   * @param extension New extension to apply
   * @return New FilePath with replaced extension
   */
  [[nodiscard]] FilePath replaceExtension(const std::wstring& extension) const;

  /**
   * @brief Check if file has one of the specified extensions
   *
   * @param extensions Vector of extensions to check against
   * @return true if file has one of the specified extensions, false otherwise
   */
  [[nodiscard]] bool hasExtension(const std::vector<std::wstring>& extensions) const;

  // Assignment Operators
  /**
   * @brief Copy assignment operator
   *
   * @param other Source FilePath to copy
   * @return Reference to current FilePath
   */
  FilePath& operator=(const FilePath& other);

  /**
   * @brief Move assignment operator
   *
   * @param other Source FilePath to move
   * @return Reference to current FilePath
   * @note Noexcept guarantee provided
   */
  FilePath& operator=(FilePath&& other) noexcept;

  bool operator==(const FilePath& other) const;

  bool operator!=(const FilePath& other) const;

  bool operator<(const FilePath& other) const;

private:
  std::unique_ptr<std::filesystem::path> m_path;

  mutable std::optional<bool> m_exists;
  mutable std::optional<bool> m_isDirectory;
  bool m_canonicalized = false;
};