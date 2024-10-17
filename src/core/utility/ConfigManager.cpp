#include "ConfigManager.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <range/v3/algorithm/replace.hpp>

#include "logging.h"
#include "TextAccess.h"
#include "utility.h"
#include "utilityString.h"

namespace {

void createXmlDocumentToString(const std::multimap<std::string, std::string>& values, std::ostream& outStream) {
  namespace pt = boost::property_tree;
  pt::ptree tree;
  for(const auto& [key, value] : values) {
    if(key.empty() || value.empty()) {
      continue;
    }
    auto newKey = "config/" + key;
    ranges::cpp20::replace(newKey, '/', '.');
    tree.add(newKey, value);
  }
  pt::write_xml(outStream, tree);
}

std::optional<std::multimap<std::string, std::string>> createStringToXmlDocument(std::istream& inStream) {
  namespace pt = boost::property_tree;
  pt::ptree tree;
  pt::read_xml(inStream, tree);

  auto configChild = tree.find("config");
  if(tree.not_found() == configChild) {
    return std::nullopt;
  }

  using NodeItr = decltype(configChild->second);

  std::multimap<std::string, std::string> output;
  std::stack<std::pair<std::string, NodeItr>> nodeStack{{{"", configChild->second}}};

  while(!nodeStack.empty()) {
    auto [currentKey, currentItem] = nodeStack.top();
    nodeStack.pop();

    if(currentItem.empty()) {
      // output.insert(std::make_pair("", currentItem.data()));
    } else {
      for(auto& [key, child] : currentItem) {
        auto newKey = currentKey.empty() ? key : fmt::format("{}/{}", currentKey, key);
        if(!child.empty()) {
          nodeStack.emplace(newKey, child);
        } else {
          output.emplace(newKey, child.data());
        }
      }
    }
  }
  return output;
}

}    // namespace

std::shared_ptr<ConfigManager> ConfigManager::createEmpty() {
  return std::shared_ptr<ConfigManager>(new ConfigManager);
}

std::shared_ptr<ConfigManager> ConfigManager::createAndLoad(const std::shared_ptr<TextAccess>& textAccess) {
  if(auto configManager = ConfigManager::createEmpty(); configManager->load(textAccess)) {
    return configManager;
  }
  return nullptr;
}

ConfigManager::ConfigManager() = default;

ConfigManager::ConfigManager(const ConfigManager&) = default;

ConfigManager::~ConfigManager() = default;

std::shared_ptr<ConfigManager> ConfigManager::createCopy() {
  return std::shared_ptr<ConfigManager>(new ConfigManager(*this));
}

void ConfigManager::clear() {
  mConfigValues.clear();
}

void ConfigManager::removeValues(const std::string& key) {
  for(const std::string& subLevelKey : getSublevelKeys(key)) {
    removeValues(subLevelKey);
  }
  mConfigValues.erase(key);
}

bool ConfigManager::isValueDefined(const std::string& key) const {
  return (mConfigValues.find(key) != mConfigValues.end());
}

std::vector<std::string> ConfigManager::getSublevelKeys(const std::string& key) const {
  std::set<std::string> keys;
  for(const auto& m_value : mConfigValues) {
    if(utility::isPrefix(key, m_value.first)) {
      size_t startPos = m_value.first.find("/", key.size());
      if(startPos == key.size()) {
        std::string subLevelKey = m_value.first.substr(0, m_value.first.find("/", startPos + 1));
        keys.insert(subLevelKey);
      }
    }
  }
  return utility::toVector(keys);
}

bool ConfigManager::load(const std::shared_ptr<TextAccess>& textAccess) {
  std::stringstream iStream{textAccess->getText()};
  if(auto result = createStringToXmlDocument(iStream); result.has_value()) {
    mConfigValues = result.value();
    return true;
  }
  return false;
}

bool ConfigManager::save(const std::filesystem::path& filepath) const {
  std::ofstream outStream{filepath};
  if(!outStream.is_open()) {
    LOG_ERROR_W(fmt::format(L"Failed to open \"{}\"", filepath.wstring()));
    return false;
  }
  createXmlDocumentToString(mConfigValues, outStream);
  return outStream.good();
}

std::string ConfigManager::toString() const {
  std::stringstream outStream;
  createXmlDocumentToString(mConfigValues, outStream);
  return outStream.str();
}