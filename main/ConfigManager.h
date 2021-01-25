#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

// Class to manage configuration files for a deployment of the Pissbot firmware
// Provides I/O functionality for Pissbot JSON configuration files

#include "PBCommon.h"
#include "DistillerManager.h"
#include <string>

class ConfigManager
{
    static constexpr const char* Name = "ConfigManager";
    static constexpr const char* FSBasePath = "/spiffs";
    static constexpr const char* FSPartitionLabel = "config";

    public:
        static PBRet loadConfig(const std::string& cfgPath, DistillerConfig& cfg); 

};

#endif // CONFIGMANAGER_H