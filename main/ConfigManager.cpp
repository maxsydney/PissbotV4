#include "ConfigManager.h"
#include "Filesystem.h"
#include "cJSON.h"
#include <fstream>
#include <sstream>
// #include <dirent.h>

PBRet ConfigManager::loadConfig(const std::string& cfgPath, DistillerConfig& cfg)
{
    // Mount filesystem
    Filesystem F(ConfigManager::FSBasePath, ConfigManager::FSPartitionLabel, 5, true);

    // Read JSON string from file
    std::ifstream configIn(cfgPath.c_str());
    if (configIn.good() == false) {
        ESP_LOGW(ConfigManager::Name, "Config file %s could not be opened", cfgPath.c_str());
        return PBRet::FAILURE;
    }

    std::stringstream JSONBuffer;
    JSONBuffer << configIn.rdbuf();

    cJSON* root = cJSON_Parse(JSONBuffer.str().c_str());
    if (root == nullptr) {
        ESP_LOGE(ConfigManager::Name, "Failed to load distiller config from JSON. Root JSON pointer was null");
        ESP_LOGE(ConfigManager::Name, "Quitting");
        return PBRet::FAILURE;
    }

    cJSON* configNode = cJSON_GetObjectItem(root, "DistillerConfig");
    return DistillerManager::loadFromJSON(cfg, configNode);
}