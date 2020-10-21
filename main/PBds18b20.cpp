#include "PBds18b20.h"

Ds18b20::Ds18b20(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, const OneWireBus* bus)
{
    if (Ds18b20::checkInputs(romCode, res, bus) != PBRet::SUCCESS) {
        ESP_LOGE(Ds18b20::Name, "ds18b20 sensor was not initialized");
        _configured = false;
    }

    // Initialize info
    ds18b20_init(&_info, bus, romCode);
    ds18b20_use_crc(&_info, true);
    ds18b20_set_resolution(&_info, res);

    // Check if sensor responds on bus
    if (ds18b20_read_resolution(&_info) == DS18B20_RESOLUTION_INVALID) {
        ESP_LOGW(Ds18b20::Name, "Device was initialised but didn't respond on bus");
        _configured = false;
    }

    ESP_LOGI(Ds18b20::Name, "Device successfully configured!");
    _configured = true;
}

PBRet Ds18b20::checkInputs(OneWireBus_ROMCode romCode, DS18B20_RESOLUTION res, const OneWireBus* bus)
{
    if (bus == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Onewire bus pointer was null. Unable to initialize sensor");
        return PBRet::FAILURE;
    }

    // TODO: Check address

    return PBRet::SUCCESS;
}

PBRet Ds18b20::readTemp(float& temp)
{
    if (_configured == false) {
        ESP_LOGW(Ds18b20::Name, "Sensor is not configured. Temperature read failed");
        return PBRet::FAILURE;
    }

    if (ds18b20_read_temp(&_info, &temp) != DS18B20_OK) {
        ESP_LOGW(Ds18b20::Name, "Temperature read failed");
        return PBRet::FAILURE;
    }

    return PBRet::SUCCESS;
}

PBRet Ds18b20::serialize(cJSON* root) const
{
    // Write sensor configuration to JSON. Sensor configuration is
    // DS18B20_Info (and maybe calibration data later)

    if (root == nullptr) {
        ESP_LOGW(Ds18b20::Name, "JSON pointer was null");
        return PBRet::FAILURE;
    }

    // All we really need is the ROM code
    cJSON* romCode = cJSON_CreateArray();
    if (romCode == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Failed to create JSON array for sensor ROM code");
        return PBRet::FAILURE;
    }

    for (int j=0; j < 8; j++) {
        cJSON* byte = cJSON_CreateNumber((int) _info.rom_code.bytes[j]);
        if (byte == nullptr) {
            ESP_LOGW(Ds18b20::Name, "Error creating byte in JSON ROM code byte array");
            cJSON_Delete(romCode);
            return PBRet::FAILURE;
        }
        cJSON_AddItemToArray(romCode, byte);
    }
    cJSON_AddItemToArray(root, romCode);

    return PBRet::SUCCESS;
}