#pragma once
// STL
#include <string>

#include <unordered_map>
// Boost
#include <boost/smart_ptr/shared_ptr.hpp>
// Qt5
#include <QObject>


namespace plugin {

struct IPlugin;
using IPluginPtr = boost::shared_ptr<IPlugin>;

/**
 * @brief Plugin mManager system
 *
 * Load and cache plugins
 */
class PluginManager final : public QObject {
  Q_OBJECT
public:
  using Ptr = std::shared_ptr<PluginManager>;

  /**
   * @brief Get/Create an instance of the class.
   *
   * @return single instance of the class.
   */
  static const Ptr& instance();

  /**
   * @brief Destructor
   */
  ~PluginManager() override;

  /**
   * @brief Load plugins in the plugin directory
   */
  void loadPlugins();

  /**
   * @brief Load a plugin
   *
   * @param plugName a full path to the plugin
   */
  void loadPlugin(std::string_view plugName);

  /**
   * @brief Check the plugin is loaded
   *
   * @param plugName a plugin name
   *
   * @return true if the plugin is loaded
   */
  bool exists(const std::string& plugName) const;

signals:
  /**
   * @brief a signal emitted when the plugin is loaded
   *
   * @param plugName a plugin name
   */
  void pluginLoaded(std::string_view plugName);

private:
  /**
   * @brief Constructor
   */
  PluginManager() noexcept;

  static Ptr s_pInstance;

  std::unordered_map<std::string, IPluginPtr> mDynamicLibraries;
};

}    // namespace plugin