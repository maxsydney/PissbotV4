#include "OneWireBus.h"
#include "esp_spiffs.h"
#include "Filesystem.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Temporary filsystem testing
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

// TODO: Move the load from JSON stuff to sensorManager

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
        _configured = false;
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

    // Read JSON string from file
    std::ifstream configIn(PBOneWire::deviceFile);
    if (configIn.good() == false) {
        ESP_LOGW(PBOneWire::Name, "Devices file could not be opened");
        return PBRet::FAILURE;
    }

    std::stringstream JSONBuffer;
    JSONBuffer << configIn.rdbuf();

    // Load JSON object
    // Just load one sensor for now. Package into proper methods for loading all sensors once
    // validated
    cJSON* configRoot = cJSON_Parse(JSONBuffer.str().c_str());
    if (configRoot == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Sensor config file was opened but could not be parsed");
        return PBRet::FAILURE;
    }

    // Load saved temperature sensors
    // No longer required to check for null, as this check is performed in 
    // cJSON_GetObjectItemCaseSensitive
    const cJSON* tempSensors = cJSON_GetObjectItemCaseSensitive(configRoot, "TempSensors");
    if (_loadTempSensorsFromJSON(tempSensors) != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "No temperature sensor config data was available");
    }

    // Load saved flowmeters
    const cJSON* flowmeters = cJSON_GetObjectItemCaseSensitive(configRoot, "Flowmeters");
    if (_loadFlowmetersFromJSON(flowmeters) != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "No flowmeter sensor config data was available");
    }

    cJSON_Delete(configRoot);
    return PBRet::SUCCESS;
}

PBRet PBOneWire::_loadTempSensorsFromJSON(const cJSON* JSONTempSensors)
{
    if (JSONTempSensors == nullptr) {
        return PBRet::FAILURE;
    }

    Ds18b20 savedSensor {};

    // If there is saved head temp sensor in the config file, load it
    const cJSON* headTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, "headTemp");
    if (headTemp != nullptr) {
        savedSensor = Ds18b20(headTemp, _cfg.tempSensorResolution, _owb);
        if (savedSensor.isConfigured() && isAvailable(savedSensor)) {
            _headTempSensor = savedSensor;
            ESP_LOGI(PBOneWire::Name, "Loaded saved head temp sensor from file");
        }
    }

    // Load reflux out temp sensor
    const cJSON* refluxTemp = cJSON_GetObjectItemCaseSensitive(JSONTempSensors, "refluxTemp");
    if (refluxTemp != nullptr) {
        savedSensor = Ds18b20(refluxTemp, _cfg.tempSensorResolution, _owb);
        if (savedSensor.isConfigured() && isAvailable(savedSensor)) {
            _refluxTempSensor = savedSensor;
            ESP_LOGI(PBOneWire::Name, "Loaded reflux outflow temp sensor from file");
        }
    }

    // TODO: Load others

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_loadFlowmetersFromJSON(const cJSON* JSONFlowmeters)
{
    if (JSONFlowmeters == nullptr) {
        return PBRet::FAILURE;
    }

    // TODO: Implement

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

    // // This is a temporary method to load a temperature sensor into the headTemp channel
    // // for testing
    // if (initialiseTempSensors() != PBRet::SUCCESS) {
    //     ESP_LOGW(PBOneWire::Name, "Sensors were not initialized");
    // }

    return PBRet::SUCCESS;
}

PBRet PBOneWire::_writeToFile(void) const
{
    // Write the current sensor configuration to JSON
    ESP_LOGI(PBOneWire::Name, "Writing OneWire sensor data to file");

    std::string JSONstr;
    if (_serialize(JSONstr) != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "Failed to write sensor config to file");
        return PBRet::FAILURE;
    }
    
    // Mount filesystem
    Filesystem F(PBOneWire::FSBasePath, PBOneWire::FSPartitionLabel, 5, true);
    if (F.isOpen() == false) {
        ESP_LOGW(PBOneWire::Name, "Failed to mount filesystem. Sensor data was not written to file");
        return PBRet::FAILURE;
    }

    std::ofstream outFile(PBOneWire::deviceFile);
    if (outFile.is_open() == false) {
        ESP_LOGE(PBOneWire::Name, "Failed to open file for writing. Sensor data was not written to file");
        return PBRet::FAILURE;
    }

    // Write JSON string out to file
    outFile << JSONstr;

    ESP_LOGI(PBOneWire::Name, "Sensor configuration successfully written to file");
    return PBRet::SUCCESS;
}

 PBRet PBOneWire::_serialize(std::string& JSONstr) const
{
    // Helper method for writing out the sensor configuration to JSON
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        ESP_LOGW(PBOneWire::Name, "Unable to create root JSON object");
        return PBRet::FAILURE;
    }

    // Create an object for temperature sensors
    cJSON* tempSensors = cJSON_CreateObject();
    if (tempSensors == NULL) {
        ESP_LOGW(PBOneWire::Name, "Unable to create tempSensors JSON object");
        return PBRet::FAILURE;
    }

    // Write individual sensors to tempSensors
    if (_headTempSensor.isConfigured()) {
        // Write out its data
        cJSON* headTemp = cJSON_CreateObject();
        if (headTemp == NULL) {
            ESP_LOGW(PBOneWire::Name, "Unable to create headTemp JSON object");
            return PBRet::FAILURE;
        }

        // TODO: Add headTemp to tempSensors and the modify inplace?
        if (_headTempSensor.serialize(headTemp) != PBRet::SUCCESS) {
            ESP_LOGW(PBOneWire::Name, "Couldn't serialize head temp sensor");
            cJSON_Delete(headTemp);
            return PBRet::FAILURE;
        }

        cJSON_AddItemToObject(tempSensors, "headTemp", headTemp);
    }
    cJSON_AddItemToObject(root, "TempSensors", tempSensors);

    // When other sensors exist, write them out here

    // Copy JSON to string. cJSON requires printing to a char* pointer. Copy into
    // std::string and free memory to avoid memory leak
    char* stringPtr = cJSON_Print(root);
    JSONstr = std::string(stringPtr);
    cJSON_Delete(root);
    free(stringPtr);

    return PBRet::SUCCESS;
} 

PBRet PBOneWire::_printConfigFile(void) const
{
    // Print config JSON file to the console. Assumes filesystem is
    // mounted

    ESP_LOGI(PBOneWire::Name, "Reading config file");

    std::ifstream JSONstream(PBOneWire::deviceFile);
    if (JSONstream.good() == false) {
        ESP_LOGW(PBOneWire::Name, "Failed to open config file for reading");
        return PBRet::FAILURE;
    }

    std::stringstream JSONBuffer;
    JSONBuffer << JSONstream.rdbuf();

    printf("%s\n", JSONBuffer.str().c_str());

    return PBRet::SUCCESS;
}

bool PBOneWire::isAvailable(const Ds18b20& sensor) const
{
    // Returns true if a ds18b20 sensor exists in the list of available 
    // sensors

    for (const Ds18b20& available : _availableSensors) {
        if (sensor == available) {
            return true;
        }
    }

    return false;
}