#include "PBds18b20.h"

Ds18b20::Ds18b20(const Ds18b20Config& config)
{
    if (_initFromConfig(config) == PBRet::SUCCESS) {
        _configured = true;
    } else {
        ESP_LOGW(Ds18b20::Name, "ds18b20 sensor was not initialized");
    }
}

Ds18b20::Ds18b20(const PBDS18B20Sensor& serialConfig, DS18B20_RESOLUTION res, const OneWireBus* bus)
{
    // Load a sensor from a protobuf definition

    Ds18b20Config config {};
    if (loadFromSerial(serialConfig, res, bus, config) == PBRet::SUCCESS)
    {
        if (_initFromConfig(config) == PBRet::SUCCESS) {
            _configured = true;
        } else {
            ESP_LOGW(Ds18b20::Name, "ds18b20 sensor was not initialized");
        }
    }
}

PBRet Ds18b20::checkInputs(const Ds18b20Config& config)
{
    if (config.bus == nullptr) {
        ESP_LOGW(Ds18b20::Name, "Onewire bus pointer was null. Unable to initialize sensor");
        return PBRet::FAILURE;
    }

    // TODO: Check address

    // TODO: Check calibration

    return PBRet::SUCCESS;
}

PBRet Ds18b20::loadFromSerial(const PBDS18B20Sensor& serialConfig, DS18B20_RESOLUTION res, 
                              const OneWireBus* bus, Ds18b20Config& config)
{
    // Copy ROM code
    OneWireBus_ROMCode romCode {};

    ESP_LOGW(Ds18b20::Name, "Loading DS18B20 sensor from serial");

    for (size_t i = 0; i < ROM_SIZE; i++) {
        romCode.bytes[i] = serialConfig.romCode()[i];
    }

    const double calibLinear = serialConfig.calibLinear();
    const double calibOffset = serialConfig.calibOffset();

    config = Ds18b20Config(romCode, calibLinear, calibOffset, res, bus);
    return PBRet::SUCCESS;
}

PBRet Ds18b20::readTemp(float& temp) const
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
    cJSON_AddItemToObject(root, "romCode", romCode);

    return PBRet::SUCCESS;
}

PBRet Ds18b20::_initFromConfig(const Ds18b20Config& config)
{
    if (Ds18b20::checkInputs(config) == PBRet::SUCCESS) {
        // Initialize info
        ds18b20_init(&_info, config.bus, config.romCode);
        ds18b20_use_crc(&_info, true);
        ds18b20_set_resolution(&_info, config.res);

        _config = config;

        // Check if sensor has been initialized correctly by checking
        // the resolution it returns
        if (ds18b20_read_resolution(&_info) != config.res) {
            ESP_LOGW(Ds18b20::Name, "Device was not initialized correctly");
            return PBRet::FAILURE;
        }

        return PBRet::SUCCESS;
    }

    return PBRet::FAILURE;
}

bool Ds18b20::operator==(const Ds18b20& other) const
{
    // Sensors are equal if their rom codes match
    bool equal = true;
    for (size_t i = 0; i < 8; i++) {
        if (_info.rom_code.bytes[i] != other._info.rom_code.bytes[i]) {
            equal = false;
            break;
        }
    }

    return equal;
}

// Factory function for generating a Protobuf definition
// of the sensor
PBDS18B20Sensor Ds18b20::toSerialConfig(void) const
{
    PBDS18B20Sensor sensor {};
    sensor.set_calibLinear(_config.calibLinear);
    sensor.set_calibOffset(_config.calibOffset);

    // Copy ROM code
    for (size_t i = 0; i < ROM_SIZE; i++) {
        sensor.mutable_romCode()[i] = _info.rom_code.bytes[i];
    }

    return sensor;
}