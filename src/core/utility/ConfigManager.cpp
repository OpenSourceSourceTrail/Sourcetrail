#include "ConfigManager.hpp"

#include <fstream>
#include <set>
#include <vector>

#include <QTextCodec>
#include <QXmlStreamWriter>

#include <tinyxml.h>

#include "logging.h"
#include "TextAccess.h"
#include "utility.h"
#include "utilityString.h"

namespace {

std::string createXmlDocumentToString(const std::multimap<std::string, std::string>& values) {
  QString output;
  QXmlStreamWriter stream(&output);
  // NOTE: setCodec is ignored while writing to QString
  // stream.setCodec("UTF-8");
  stream.setAutoFormatting(true);

  stream.writeStartDocument();
  if(!values.empty()) {
    stream.writeStartElement("config");
    {
      for(const auto& [key, value] : values) {
        if(key.empty() || value.empty()) {
          continue;
        }
        auto tokens = QString::fromStdString(key).split('/');
        for(auto itr = tokens.begin(); itr != std::prev(tokens.end()); ++itr) {
          stream.writeStartElement(*itr);
        }
        stream.writeTextElement(tokens.back(), QString::fromStdString(value));
        for(auto index = 0; index < tokens.size() - 1; ++index) {
          stream.writeEndElement();
        }
      }
    }
    stream.writeEndElement();
  }
  stream.writeEndDocument();

  return output.toStdString();
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
  for(const std::string& sublevelKey : getSublevelKeys(key)) {
    removeValues(sublevelKey);
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
        std::string sublevelKey = m_value.first.substr(0, m_value.first.find("/", startPos + 1));
        keys.insert(sublevelKey);
      }
    }
  }
  return utility::toVector(keys);
}

bool ConfigManager::load(const std::shared_ptr<TextAccess>& textAccess) {
  TiXmlDocument doc;
  if(nullptr == doc.Parse(textAccess->getText().c_str(), nullptr, TIXML_ENCODING_UTF8)) {
    LOG_ERROR("Unable to load a XML");
    return false;
  }

  TiXmlHandle docHandle(&doc);
  TiXmlNode* rootNode = docHandle.FirstChild("config").ToNode();
  if(nullptr == rootNode) {
    LOG_ERROR("No 'config' in the configfile");
    return false;
  }

  for(TiXmlNode* childNode = rootNode->FirstChild(); nullptr != childNode; childNode = childNode->NextSibling()) {
    parseSubtree(childNode, "");
  }
  return true;
}

bool ConfigManager::save(const std::filesystem::path& filepath) const {
  std::string output = createXmlDocumentToString(mConfigValues);
  std::ofstream outStream{filepath};
  if(!outStream.is_open()) {
    LOG_ERROR_W(fmt::format(L"Failed to open \"{}\"", filepath.wstring()));
    return false;
  }
  outStream.write(output.data(), static_cast<long>(output.size()));
  return outStream.good();
}

void ConfigManager::parseSubtree(TiXmlNode* currentNode, const std::string& currentPath) {
  if(currentNode->Type() == TiXmlNode::TINYXML_TEXT) {
    std::string key = currentPath.substr(0, currentPath.size() - 1);
    mConfigValues.insert(std::pair<std::string, std::string>(key, currentNode->ToText()->Value()));
  } else {
    for(TiXmlNode* childNode = currentNode->FirstChild(); childNode != nullptr; childNode = childNode->NextSibling()) {
      parseSubtree(childNode, currentPath + std::string(currentNode->Value()) + "/");
    }
  }
}

std::string ConfigManager::toString() const {
  return createXmlDocumentToString(mConfigValues);
}