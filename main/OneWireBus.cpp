#include "OneWireBus.h"
#include "esp_spiffs.h"

// Temporary filsystem testing
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

PBOneWire::PBOneWire(gpio_num_t OWBPin)
{
    // Mount PBData filesystem
    if (_mountFS() != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "File system was not mounted");
    }

    // Check input parameters
    if (checkInputs(OWBPin) != PBRet::SUCCESS) {
        ESP_LOGE(PBOneWire::Name, "Failed to configure OneWire bus");
        _configured = false;
    }

    // Initialize the bus
    if (_initOWB(OWBPin) != PBRet::SUCCESS) {
        ESP_LOGE(PBOneWire::Name, "Failed to configure OneWire bus");
        _configured = false;
    }

    scanForDevices();

    if (_connectedDevices <= 0) {
        ESP_LOGW(PBOneWire::Name, "No devices were found on OneWire bus");
    }

    ESP_LOGI(PBOneWire::Name, "Onewire bus configured on pin %d", OWBPin);
    _configured = true;
}

PBRet PBOneWire::checkInputs(gpio_num_t OWPin)
{
    // Check pin is valid GPIO
    if ((OWPin <= GPIO_NUM_NC) || (OWPin > GPIO_NUM_MAX)) {
        ESP_LOGE(PBOneWire::Name, "OneWire pin %d is invalid. Bus was not configured", OWPin);
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_initOWB(gpio_num_t OWPin)
{
    _owb = owb_rmt_initialize(&_rmt_driver_info, OWPin, RMT_CHANNEL_1, RMT_CHANNEL_0);

    if (_owb == nullptr) {
        ESP_LOGE(PBOneWire::Name, "OnewWireBus initialization returned nullptr");
        return PBRet::FAILURE;
    }

    // Enable CRC check for ROM code. The return type from this method indicates 
    // if bus was configured correctly
    if (owb_use_crc(_owb, true) != OWB_STATUS_OK) {
        ESP_LOGE(PBOneWire::Name, "OnewWireBus initialization failed");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_mountFS(void) const
{
    // Mount a spiffs file system in the PBData partition for
    // reading and storing known sensor data

    ESP_LOGI(PBOneWire::Name, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(PBOneWire::Name, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(PBOneWire::Name, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(PBOneWire::Name, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return PBRet::FAILURE;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(PBOneWire::Name, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(PBOneWire::Name, "Partition size: total: %d, used: %d", total, used);
    }

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(PBOneWire::Name, "Opening file");
    FILE* f = fopen("/spiffs/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(PBOneWire::Name, "Failed to open file for writing");
        return PBRet::FAILURE;
    }
    fprintf(f, "Hello World!\n");
    fclose(f);
    ESP_LOGI(PBOneWire::Name, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/spiffs/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/spiffs/foo.txt");
    }

    // Rename original file
    ESP_LOGI(PBOneWire::Name, "Renaming file");
    if (rename("/spiffs/hello.txt", "/spiffs/foo.txt") != 0) {
        ESP_LOGE(PBOneWire::Name, "Rename failed");
        return PBRet::FAILURE;
    }

    // Open renamed file for reading
    ESP_LOGI(PBOneWire::Name, "Reading file");
    f = fopen("/spiffs/foo.txt", "r");
    if (f == NULL) {
        ESP_LOGE(PBOneWire::Name, "Failed to open file for reading");
        return PBRet::FAILURE;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(PBOneWire::Name, "Read from file: '%s'", line);

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(PBOneWire::Name, "SPIFFS unmounted");

    return PBRet::SUCCESS;
}

PBRet PBOneWire::scanForDevices(void)
{
    if (_owb == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Onewire bus pointer was null");
        return PBRet::FAILURE;
    }

    OneWireBus_SearchState search_state {};
    bool found = false;
    _connectedDevices = 0;
    _availableRomCodes.clear();

    owb_search_first(_owb, &search_state, &found);
    while (found) {
        _availableRomCodes.push_back(search_state.rom_code);
        _connectedDevices++;
        owb_search_next(_owb, &search_state, &found);
    }

    return PBRet::SUCCESS;
}