#include "OneWireBus.h"
#include "esp_spiffs.h"
#include "Filesystem.h"

// Temporary filsystem testing
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

PBOneWire::PBOneWire(const PBOneWireConfig& cfg)
{
    // Check input parameters
    if (checkInputs(cfg) != PBRet::SUCCESS) {
        ESP_LOGE(PBOneWire::Name, "Failed to configure OneWire bus");
        _configured = false;
    }

    // Initialize PBOneWire
    if (_initFromParams(cfg) == PBRet::SUCCESS) {
        ESP_LOGI(PBOneWire::Name, "PBOneWire configured!");
        _configured = true;
    } else {
        ESP_LOGW(PBOneWire::Name, "Unable to configure PBOneWire");
    }
}

PBRet PBOneWire::checkInputs(const PBOneWireConfig& cfg)
{
    // Check pin is valid GPIO
    if ((cfg.oneWirePin <= GPIO_NUM_NC) || (cfg.oneWirePin > GPIO_NUM_MAX)) {
        ESP_LOGE(PBOneWire::Name, "OneWire pin %d is invalid. Bus was not configured", cfg.oneWirePin);
        return PBRet::FAILURE;
    }

    if ((cfg.refluxFlowPin <= GPIO_NUM_NC) || (cfg.refluxFlowPin > GPIO_NUM_MAX)) {
        ESP_LOGE(PBOneWire::Name, "Reflux flowmeter GPIO %d is invalid. SensorManager was not configured", cfg.refluxFlowPin);
        return PBRet::FAILURE;
    }

    if ((cfg.productFlowPin <= GPIO_NUM_NC) || (cfg.productFlowPin > GPIO_NUM_MAX)) {
        ESP_LOGE(PBOneWire::Name, "Product flowmeter GPIO %d is invalid. SensorManager was not configured", cfg.productFlowPin);
        return PBRet::FAILURE;
    }

    if (cfg.tempSensorResolution == DS18B20_RESOLUTION_INVALID) {
        ESP_LOGE(PBOneWire::Name, "Temperature sensor resolution was invalid");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_initOWB()
{
    _rmtDriver = new owb_rmt_driver_info;
    if (_rmtDriver == nullptr) {
        ESP_LOGE(PBOneWire::Name, "Unable to allocate memory for RMT driver");
        return PBRet::FAILURE;
    }

    // Fields are statically allocated within owb_rmt_initialize
    _owb = owb_rmt_initialize(_rmtDriver, _cfg.oneWirePin, RMT_CHANNEL_1, RMT_CHANNEL_0);
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

PBRet PBOneWire::_loadKnownDevices(const char* basePath, const char* partitionLabel)
{
    // Nullptr check
    if ((basePath == nullptr) || (partitionLabel == nullptr)) {
        ESP_LOGW(PBOneWire::Name, "Cannot load known devices. One or more input pointers were null");
        return PBRet::FAILURE;
    }

    // Mount filesystem
    Filesystem F(PBOneWire::FSBasePath, PBOneWire::FSPartitionLabel, 5, true);

    if (F.isOpen()) {
        // Check if devices file exists
        struct stat st;
        if (stat(PBOneWire::deviceFile, &st) == 0) {
            ESP_LOGI(PBOneWire::Name, "Devices file found");
            _printConfigFile();
        } else {
            ESP_LOGW(PBOneWire::Name, "No devices file was found");
            return PBRet::FAILURE;
        }
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::scanForDevices(void)
{
    if (_owb == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Onewire bus pointer was null");
        return PBRet::FAILURE;
    }

    if (xSemaphoreTake(_OWBMutex, 250 / portTICK_PERIOD_MS) == pdTRUE) {
        OneWireBus_SearchState search_state {};
        bool found = false;
        _connectedDevices = 0;
        _availableSensors.clear();

        owb_search_first(_owb, &search_state, &found);
        while (found) {
            _availableSensors.emplace_back(Ds18b20(search_state.rom_code, _cfg.tempSensorResolution, _owb));
            _connectedDevices++;
            owb_search_next(_owb, &search_state, &found);
        }
        xSemaphoreGive(_OWBMutex);
    } else {
        ESP_LOGW(PBOneWire::Name, "(scanForDevices) Unable to access PBOneWire shared resource");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::initialiseTempSensors(void)
{
    // Connect to sensors and create PBds18b20 object
    if (_connectedDevices == 0) {
        ESP_LOGW(PBOneWire::Name, "No available devices");
        return PBRet::FAILURE;
    }

    // NOTE: Just assigning the first for now. Assign properly from saved sensors JSON
    // Assign sensors
    _headTempSensor = _availableSensors.at(0);
    if (_writeToFile() != PBRet::SUCCESS) {
        return PBRet::FAILURE;
    }

    // Connect other devices here
    
    return PBRet::SUCCESS;
}

PBRet PBOneWire::_oneWireConvert(void) const
{
    // Command all temperature sensors on the bus to convert temperatures.
    // NOTE: This method is not protected by a semaphore. If you are calling
    //       this method, it is your responsibility to make sure that the bus
    //       is not locked out

    if (_connectedDevices == 0) {
        ESP_LOGW(PBOneWire::Name, "No available devices. Cannot convert temperatures");
        return PBRet::FAILURE;
    }

    ds18b20_convert(&_headTempSensor.getInfo());
    ds18b20_wait_for_conversion(&_headTempSensor.getInfo());

    return PBRet::SUCCESS;
}

PBRet PBOneWire::readTempSensors(TemperatureData& Tdata)
{
    // Read all available temperature sensors
    float headTemp = 0.0;

    if (_configured == false) {
        ESP_LOGW(PBOneWire::Name, "Cannot read temperatures before PBOneWireBuse is configured");
        return PBRet::FAILURE;
    }

    if (xSemaphoreTake(_OWBMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        if (_oneWireConvert() != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Temperature sensor conversion failed");
            xSemaphoreGive(_OWBMutex);
            return PBRet::FAILURE;
        }

        if (_headTempSensor.readTemp(headTemp) != PBRet::SUCCESS) {
            // Error message printed in readTemp
            xSemaphoreGive(_OWBMutex);
            return PBRet::FAILURE;
        }
        xSemaphoreGive(_OWBMutex);
    } else {
        ESP_LOGW(PBOneWire::Name, "Unable to access PBOneWire shared resource");
        return PBRet::FAILURE;
    }

    // Read other sensors here

    // Fill data structure
    Tdata = TemperatureData(headTemp, 0.0, 0.0, 0.0, 0.0);

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_initFromParams(const PBOneWireConfig& cfg)
{
    _cfg = cfg;

    // Initialize the bus
    if (_initOWB() != PBRet::SUCCESS) {
        ESP_LOGE(PBOneWire::Name, "Failed to configure OneWire bus");
        return PBRet::FAILURE;
    }

    // Initialise bus semaphore
    _OWBMutex = xSemaphoreCreateMutex();
    if (_OWBMutex == NULL) {
        ESP_LOGW(PBOneWire::Name, "Unable to create mutex");
        return PBRet::FAILURE;
    }

    // The following cases can fail and be recovered from. Don't return failure
    // Find available devices on bus
    scanForDevices();
    if (_connectedDevices <= 0) {
        ESP_LOGW(PBOneWire::Name, "No devices were found on OneWire bus");
    }

    // Load saved devices
    if (_loadKnownDevices(PBOneWire::FSBasePath, PBOneWire::FSPartitionLabel) != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "No saved devices were found");
    }

    if (initialiseTempSensors() != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "Sensors were not initialized");
    }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_writeToFile(void) const
{
    // Write the current sensor configuration to JSON
    ESP_LOGI(PBOneWire::Name, "Writing OneWire sensor data to file");

    char* JSONstr = _serialize();
    if (JSONstr == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Failed to write sensor config to file");
        return PBRet::FAILURE;
    }
    
    // Mount filesystem
    Filesystem F(PBOneWire::FSBasePath, PBOneWire::FSPartitionLabel, 5, true);
    if (F.isOpen() == false) {
        ESP_LOGW(PBOneWire::Name, "Failed to mount filesystem. Sensor data was not written to file");
        free(JSONstr);
        return PBRet::FAILURE;
    }

    // Check if devices file exists
    struct stat st;
    if (stat(PBOneWire::deviceFile, &st) == 0) {
        ESP_LOGI(PBOneWire::Name, "Existing file will be overwritten");
    } else {
        ESP_LOGI(PBOneWire::Name, "No device file exists. Creating new file");
    }

    FILE* f = fopen(PBOneWire::deviceFile, "w");
    if (f == NULL) {
        ESP_LOGE(PBOneWire::Name, "Failed to open file for writing. Sensor data was not written to file");
        fclose(f);
        free(JSONstr);
        return PBRet::FAILURE;
    }

    if (fputs(JSONstr, f) == EOF) {
        ESP_LOGE(PBOneWire::Name, "Error writing JSON string to file");
        fclose(f);
        free(JSONstr);
        return PBRet::FAILURE;
    }

    ESP_LOGI(PBOneWire::Name, "Successfully wrote sensor config to JSON file");

    // Success by here
    fclose(f);
    free(JSONstr);
    return PBRet::SUCCESS;
}

char* PBOneWire::_serialize(void) const
{
    // Helper method for writing out the sensor configuration to JSON
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Unable to create root JSON object");
        return nullptr;
    }

    if (root == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Unable to create root JSON object");
        return nullptr;
    }

    // Create an object for temperature sensors
    cJSON* tempSensors = cJSON_CreateObject();
    if (tempSensors == NULL) {
        ESP_LOGW(PBOneWire::Name, "Unable to create tempSensors JSON object");
        return nullptr;
    }

    cJSON_AddItemToObject(root, "TempSensors", tempSensors);

    // Write individual sensors to tempSensors
    if (_headTempSensor.isConfigured()) {
        // Write out its data
        cJSON* headTemp = cJSON_CreateObject();
        if (headTemp == NULL) {
            ESP_LOGW(PBOneWire::Name, "Unable to create headTemp JSON object");
            return nullptr;
        }

        // TODO: Add headTemp to tempSensors and the modify inplace?
        if (_headTempSensor.serialize(headTemp) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Couldn't serialize head temp sensor");
            cJSON_Delete(headTemp);
            return nullptr;
        }

        cJSON_AddItemToObject(tempSensors, "headTemp", headTemp);
    }

    // When other sensors exist, write them out here

    // Write JSON string to input pointer and free memory
    char* JSONstr = cJSON_Print(root);
    cJSON_Delete(root);

    return JSONstr;
} 

PBRet PBOneWire::_printConfigFile(void) const
{
    // Print config JSON file to the console. Assumes filesystem is
    // mounted

    ESP_LOGI(PBOneWire::Name, "Reading config file");
    FILE* f = fopen(PBOneWire::deviceFile, "r");
    if (f == NULL) {
        ESP_LOGW(PBOneWire::Name, "Failed to open config file for reading");
        return PBRet::FAILURE;
    }

    // Stack allocation for now
    char config[1024] = {0};
    fgets(config, sizeof(config), f);
    fclose(f);

    ESP_LOGI(PBOneWire::Name, "%s", config);

    return PBRet::SUCCESS;
}