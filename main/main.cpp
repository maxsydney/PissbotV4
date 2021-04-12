#include <esp_log.h>
#include "nvs_flash.h"
#include "DistillerManager.h"
#include "ConfigManager.h"

#ifdef __cplusplus
extern "C" {
#endif

int _log_vprintf(const char *fmt, va_list args);

void app_main()
{
    ESP_LOGI("Main", "Redirecting log messages to websocket connection");
    esp_log_set_vprintf(&_log_vprintf);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    DistillerConfig cfg {};

    if (ConfigManager::loadConfig("/spiffs/PissbotConfig.json", cfg) != PBRet::SUCCESS) {
        ESP_LOGE("Main", "Failed to configure system");
    } else {
        DistillerManager* manager = DistillerManager::getInstance(5, 16384, 1, cfg);
        manager->begin();
    }
}

int _log_vprintf(const char *fmt, va_list args) 
{
    // Handle redirection of logging to websockets
    char logBuff[512] {};
    int iresult = 0;

    iresult = vsnprintf(logBuff, 511, fmt, args);
    if (iresult < 0) {
        printf("%s() ABORT. failed vfprintf() -> disable future vfprintf(_log_remote_fp) \n", __FUNCTION__);
        return iresult;
    }

    // Log to all connected websockets
    Webserver::socketLog(logBuff);
    
    // Log to stdout
    return vprintf(fmt, args);
}

#ifdef __cplusplus
}
#endif