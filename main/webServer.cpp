
#include "Webserver.h"
#include "connectionManager.h"
#include "espfs.h"
#include "espfs_image.h"
#include "libesphttpd/httpd-espfs.h"
#include "esp_netif.h"
#include "libesphttpd/route.h"

Webserver::Webserver(const WebserverConfig& cfg)
{
    // Init from params
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(Webserver::Name, "Webserver configured!");
        _configured = true;
    } else {
        ESP_LOGW(Webserver::Name, "Unable to configure Webserver");
    }
}

PBRet Webserver::_initFromParams(const WebserverConfig& cfg)
{
    if (Webserver::checkInputs(cfg) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    if (_startupWebserver(cfg) == PBRet::FAILURE) {
        return PBRet::FAILURE;
    }

    _cfg = cfg;
    return PBRet::SUCCESS;
}

PBRet Webserver::checkInputs(const WebserverConfig& cfg)
{
    if (cfg.maxConnections <= 0) {
        ESP_LOGW(Webserver::Name, "Maximum connections must be greater than 0");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

static void openConnection(Websock *ws) 
{
    // TODO: Implement socket server task
	ESP_LOGI("Webserver", "Got connection request");
}

static HttpdBuiltInUrl builtInUrls[] = {
	ROUTE_REDIRECT("/", "/index.html"),
    ROUTE_WS("/ws", openConnection),
	ROUTE_FILESYSTEM(),
	ROUTE_END()
};

PBRet Webserver::_startupWebserver(const WebserverConfig& cfg)
{
    // Initialize connectionManager
    _connManager = ConnectionManager(_cfg.maxConnections);
    if (_connManager.isConfigured() == false) {
        ESP_LOGE(Webserver::Name, "Connection manager was not initialized");
        return PBRet::FAILURE;
    }

    // Allocate connection memory
    ESP_LOGI(Webserver::Name, "Allocating %d bytes for webserver connection memory", sizeof(RtosConnType) * cfg.maxConnections);
    connectionMemory = (RtosConnType*) malloc(sizeof(RtosConnType) * cfg.maxConnections);
    if (connectionMemory == nullptr) {
        // TODO: In the case of failure, set flag to run without webserver
        ESP_LOGE(Webserver::Name, "Failed to allocate memory for webserver");
        return PBRet::FAILURE;
    }

    // Configure webserver
	EspFsConfig espfs_conf {};
    espfs_conf.memAddr = espfs_image_bin;
	EspFs* fs = espFsInit(&espfs_conf);
    if (fs == nullptr) {
        ESP_LOGE(Webserver::Name, "Failed to initialise espfs file system");
        return PBRet::FAILURE;
    }
    
    httpdRegisterEspfs(fs);
	httpdFreertosInit(&_httpdFreertosInstance,
	                  builtInUrls,
	                  LISTEN_PORT,
	                  connectionMemory,
	                  cfg.maxConnections,
	                  HTTPD_FLAG_NONE);
	httpdFreertosStart(&_httpdFreertosInstance);

    ESP_LOGI(Webserver::Name, "Webserver initialized");
    return PBRet::SUCCESS;
}