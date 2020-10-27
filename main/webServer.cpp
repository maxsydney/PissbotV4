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
    if (checkInputs(cfg) != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    if (_startupWebserver() == PBRet::FAILURE) {
        return PBRet::FAILURE;
    }

    _cfg = cfg;
    return PBRet::SUCCESS;
}

static HttpdBuiltInUrl builtInUrls[] = {
	ROUTE_REDIRECT("/", "/index.tpl"),
	ROUTE_FILESYSTEM(),
	ROUTE_END()
};

PBRet Webserver::_startupWebserver(void)
{
	EspFsConfig espfs_conf = {
		.memAddr = espfs_image_bin,
	};
	EspFs* fs = espFsInit(&espfs_conf);
    httpdRegisterEspfs(fs);

	esp_netif_init();
	httpdFreertosInit(&_httpdFreertosInstance,
	                  builtInUrls,
	                  LISTEN_PORT,
	                  connectionMemory,
	                  ConnectionManager::MAXCONNECTIONS,
	                  HTTPD_FLAG_NONE);
	httpdFreertosStart(&httpdFreertosInstance);
}
