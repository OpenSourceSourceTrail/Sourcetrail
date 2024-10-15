#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "FilePath.h"
#include "utilityString.h"

class TextAccess;
class TiXmlNode;

/**
 * @class ConfigManager
 * @brief Manages configuration data, allowing to load, save, and manipulate key-value pairs.
 *
 * The ConfigManager class handles configuration data by providing an interface to
 * load configurations from a source, retrieve and set key-value pairs, and save the configuration.
 * It supports various data types and allows for converting between strings and typed values.
 *
 * This class cannot be copied or moved.
 */
class ConfigManager final {
public:
  /**
   * @brief A shared pointer type alias for ConfigManager.
   */
  using Ptr = std::shared_ptr<ConfigManager>;
  /**
   * @brief Creates an empty configuration manager.
   *
   * @return A shared pointer to an empty ConfigManager.
   */
  static Ptr createEmpty();
  /**
   * @brief Creates a ConfigManager and loads data from a text access object.
   *
   * @param textAccess A shared pointer to a TextAccess object containing the configuration data.
   * @return A shared pointer to the created ConfigManager.
   */
  static Ptr createAndLoad(const std::shared_ptr<TextAccess>& textAccess);

  /**
   * @brief Converts a string to a specific type.
   *
   * @tparam T The type to convert the string to.
   * @param value The string value to convert.
   * @return The converted value of type T.
   */
  template <typename T>
  static T fromString(const std::string& value) {
    if constexpr(std::is_same_v<T, std::string>) {
      return value;
    } else if constexpr(std::is_same_v<T, std::wstring>) {
      return utility::decodeFromUtf8(value);
    } else if constexpr(std::is_same_v<T, int>) {
      try {
        return std::stoi(value);
      } catch([[maybe_unused]] const std::invalid_argument& ex) {    // NOLINT
      }
    } else if constexpr(std::is_same_v<T, float>) {
      try {
        return std::stof(value);
      } catch([[maybe_unused]] const std::invalid_argument& ex) {    // NOLINT
      }
    } else if constexpr(std::is_same_v<T, bool>) {
      try {
        return std::stoi(value) != 0;
      } catch([[maybe_unused]] const std::invalid_argument& ex) {    // NOLINT
      }
    } else if constexpr(std::is_same_v<T, FilePath>) {
      return FilePath {value};
    }
    return {};
  }

  /**
   * @brief Converts a value of type T to a string.
   *
   * @tparam T The type of the value to convert to string.
   * @param value The value to convert.
   * @return The string representation of the value.
   */
  template <typename T>
  static std::string toString(const T& value) {
    std::string valueString;
    if constexpr(std::is_same_v<T, std::string>) {
      valueString = value;
    } else if constexpr(std::is_same_v<T, std::wstring>) {
      valueString = utility::encodeToUtf8(value);
    } else if constexpr(std::is_same_v<T, int> || std::is_same_v<T, float>) {
      valueString = std::to_string(value);
    } else if constexpr(std::is_same_v<T, bool>) {
      valueString = value ? '1' : '0';
    } else if constexpr(std::is_same_v<T, FilePath>) {
      valueString = value.str();
    }
    return valueString;
  }

  ConfigManager(ConfigManager&&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;
  ConfigManager& operator=(ConfigManager&&) = delete;

  /**
   * @brief Destructor for ConfigManager.
   */
  ~ConfigManager();

  /**
   * @brief Creates a copy of the current ConfigManager.
   *
   * @return A shared pointer to the copied ConfigManager.
   */
  Ptr createCopy();

  /**
   * @brief Clears all configuration values.
   */
  void clear();

  /**
   * @brief Retrieves the value associated with a given key.
   *
   * @tparam T The type of the value to retrieve.
   * @param key The key for the value to retrieve.
   * @return An optional containing the value if found, or std::nullopt if not found.
   */
  template <typename T = std::string>
  [[nodiscard]] std::optional<T> getValue(const std::string& key) const;

  /**
   * @brief Retrieves the value associated with a given key, or returns a default value if not found.
   *
   * @tparam T The type of the value to retrieve.
   * @param key The key for the value to retrieve.
   * @param defaultValue The default value to return if the key is not found.
   * @return The value associated with the key or the default value.
   */
  template <typename T>
  [[nodiscard]] T getValueOrDefault(const std::string& key, T defaultValue) const;

  /**
   * @brief Retrieves multiple values associated with a given key.
   *
   * @tparam T The type of the values to retrieve.
   * @param key The key for the values to retrieve.
   * @return An optional containing a vector of values if found, or std::nullopt if not found.
   */
  template <typename T = std::string>
  [[nodiscard]] std::optional<std::vector<T>> getValues(const std::string& key) const;

  /**
   * @brief Retrieves multiple values associated with a given key, or returns default values if not found.
   *
   * @tparam T The type of the values to retrieve.
   * @param key The key for the values to retrieve.
   * @param defaultValues The default values to return if the key is not found.
   * @return A vector of values associated with the key or the default values.
   */
  template <typename T>
  [[nodiscard]] std::vector<T> getValuesOrDefaults(const std::string& key, std::vector<T> defaultValues) const;

  /**
   * @brief Sets a value for a given key.
   *
   * @tparam T The type of the value.
   * @param key The key for the value to set.
   * @param value The value to set.
   */
  template <typename T = std::string>
  void setValue(const std::string& key, const T& value);

  /**
   * @brief Sets multiple values for a given key.
   *
   * @tparam T The type of the values.
   * @param key The key for the values to set.
   * @param values A vector of values to set.
   */
  template <typename T = std::string>
  void setValues(const std::string& key, const std::vector<T>& values);

  /**
   * @brief Removes values associated with a given key.
   *
   * @param key The key for the values to remove.
   */
  void removeValues(const std::string& key);

  /**
   * @brief Checks if a value is defined for a given key.
   *
   * @param key The key to check.
   * @return true if a value is defined, false otherwise.
   */
  [[nodiscard]] bool isValueDefined(const std::string& key) const;

  /**
   * @brief Retrieves sublevel keys for a given key.
   *
   * @param key The key for which to get sublevel keys.
   * @return A vector of sublevel keys.
   */
  [[nodiscard]] std::vector<std::string> getSublevelKeys(const std::string& key) const;

  /**
   * @brief Loads the configuration from a text access object.
   *
   * @param textAccess A shared pointer to the TextAccess object to load from.
   * @return true if loading succeeded, false otherwise.
   */
  [[nodiscard]] bool load(const std::shared_ptr<TextAccess>& textAccess);

  /**
   * @brief Saves the configuration to a file.
   *
   * @param filepath The path to the file to save the configuration.
   * @return true if saving succeeded, false otherwise.
   */
  [[nodiscard]] bool save(const std::filesystem::path& filepath) const;

  /**
   * @brief Converts the configuration data to a string representation.
   *
   * @return A string representing the configuration data.
   */
  [[nodiscard]] std::string toString() const;

private:
  ConfigManager();
  ConfigManager(const ConfigManager&);

  /**
   * @brief Parses an XML subtree and populates the configuration.
   *
   * @param parentElement The parent XML element.
   * @param currentPath The current path within the XML hierarchy.
   */
  void parseSubtree(TiXmlNode* parentElement, const std::string& currentPath);

  /// Holds the configuration values in a multimap structure.
  std::multimap<std::string, std::string> mConfigValues;
};

template <typename T>
std::optional<T> ConfigManager::getValue(const std::string& key) const {
  if(auto iterator = mConfigValues.find(key); iterator != mConfigValues.end()) {
    return fromString<T>(iterator->second);
  }
  return std::nullopt;
}

template <typename T>
std::optional<std::vector<T>> ConfigManager::getValues(const std::string& key) const {
  if(const auto& [first, last] = mConfigValues.equal_range(key); first != last) {
    std::vector<T> result;
    result.reserve(static_cast<size_t>(std::distance(first, last)));
    for(auto cit = first; cit != last; ++cit) {
      result.push_back(fromString<T>(cit->second));
    }
    return result;
  }
  return std::nullopt;
}

template <typename T>
void ConfigManager::setValue(const std::string& key, const T& value) {
  auto iterator = mConfigValues.find(key);

  std::string valueString = toString(value);
  if(iterator != mConfigValues.end()) {
    iterator->second = valueString;
  } else {
    mConfigValues.emplace(key, valueString);
  }
}

template <typename T>
void ConfigManager::setValues(const std::string& key, const std::vector<T>& values) {
  if(auto iterator = mConfigValues.find(key); iterator != mConfigValues.end()) {
    mConfigValues.erase(key);
  }
  for(const T& value : values) {
    mConfigValues.emplace(key, toString(value));
  }
}

template <typename T>
T ConfigManager::getValueOrDefault(const std::string& key, T defaultValue) const {
  if(auto result = getValue<T>(key); result) {
    return result.value();
  }
  return defaultValue;
}

template <typename T>
std::vector<T> ConfigManager::getValuesOrDefaults(const std::string& key, std::vector<T> defaultValues) const {
  if(auto values = getValues<T>(key); values.has_value()) {
    return values.value();
  }
  return defaultValues;
}
