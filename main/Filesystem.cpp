#include "Filesystem.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

Filesystem::Filesystem(const char* base_path, const char* partition_label, 
                       size_t max_files, bool format_if_mount_failed)
{
    // Attempt to open the spiffs virtual file system with the arguments
    // provided

    ESP_LOGI(Filesystem::Name, "Mounting filesystem");

    _conf = {
        base_path,
        partition_label,
        max_files,
        format_if_mount_failed
    };

    esp_err_t ret = esp_vfs_spiffs_register(&_conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(Filesystem::Name, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(Filesystem::Name, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(Filesystem::Name, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        
        _isOpen = false;
    }

    ESP_LOGI(Filesystem::Name, "Filesystem mounted");

    // If we reach here, filesystem is mounted and is ready to be used
    _isOpen = true;
}

Filesystem::~Filesystem(void)
{
    // Unmount the filesystem
    ESP_LOGI(Filesystem::Name, "Unmounting filesystem");
    esp_vfs_spiffs_unregister(_conf.partition_label);
}

PBRet Filesystem::getInfo(size_t& total, size_t& used)
{
    esp_err_t ret = esp_spiffs_info(_conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(Filesystem::Name, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}