#include "OneWireBus.h"
#include "esp_spiffs.h"
#include "Filesystem.h"

// Temporary filsystem testing
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

PBOneWire::PBOneWire(gpio_num_t OWBPin)
{
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

    // Find available devices on bus
    scanForDevices();
    if (_connectedDevices <= 0) {
        ESP_LOGW(PBOneWire::Name, "No devices were found on OneWire bus");
    }

    // Load saved devices
    if (_loadKnownDevices(PBOneWire::FSBasePath, PBOneWire::FSPartitionLabel) != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "No saved devices were found");
    }

    // Attempt to connect to sensor
    connect();

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
            // TODO: Read saved devices file
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

PBRet PBOneWire::connect(void)
{
    // Test function to connect sensor
    if (_connectedDevices == 0) {
        ESP_LOGW(PBOneWire::Name, "No available devices");
        return PBRet::FAILURE;
    }

    for (const OneWireBus_ROMCode& romCode : _availableRomCodes) {
        Ds18b20 p(romCode, DS18B20_RESOLUTION_11_BIT, _owb);
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

    if (_connectedDevices >= 1) {
        // Create a temperature sensor object and assign to main temp sensor
        OneWireBus_ROMCode romCode = _availableRomCodes.at(0);
        _headTempSensor = Ds18b20(romCode, DS18B20_RESOLUTION_11_BIT, _owb);

        if (_headTempSensor.isConfigured() == false) {
            return PBRet::FAILURE;
        }
    }

    // Connect other devices here
    
    return PBRet::SUCCESS;
}

PBRet PBOneWire::_oneWireConvert(void) const
{
    // Command all temperature sensors on the bus to convert temperatures
    if (_connectedDevices == 0) {
        ESP_LOGW(PBOneWire::Name, "No available devices. Cannot convert temperatures");
        return PBRet::FAILURE;
    }

    ds18b20_convert_all(_owb);
    ds18b20_wait_for_conversion(&_headTempSensor.getInfo());

    return PBRet::SUCCESS;
}

PBRet PBOneWire::readTempSensors(TemperatureData& Tdata)
{
    // Read all available temperature sensors
    float headTemp = 0.0;

    if (_oneWireConvert() != PBRet::SUCCESS) {
        ESP_LOGW(PBOneWire::Name, "Temperature sensor conversion failed");
        return PBRet::FAILURE;
    }

    if (_headTempSensor.readTemp(headTemp) != PBRet::SUCCESS) {
        // Error message printed in readTemp
        return PBRet::FAILURE;
    }

    // Read other sensors here

    // Fill data structure
    Tdata = TemperatureData(headTemp, 0.0, 0.0, 0.0, 0.0);

    return PBRet::SUCCESS;
}