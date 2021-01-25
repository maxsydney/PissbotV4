#include "ConfigManager.h"
#include "Filesystem.h"
#include "cJSON.h"
#include <fstream>
#include <sstream>
#include <dirent.h>

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

    printf("%s\n", JSONBuffer.str().c_str());

    return PBRet::SUCCESS;
}