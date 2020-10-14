#include "OneWireBus.h"

PBOneWire::PBOneWire(gpio_num_t OWBPin)
{
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